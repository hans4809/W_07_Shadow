#include "DepthPrePass.h"

#include "EditorEngine.h"
#include "BaseGizmos/GizmoBaseComponent.h"
#include "Components/Mesh/StaticMesh.h"
#include "D3D11RHI/CBStructDefine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Math/JungleMath.h"
#include "Renderer/VBIBTopologyMapping.h"
#include "UnrealEd/EditorViewportClient.h"

FDepthPrePass::FDepthPrePass(const FName& InShaderName)
    : FBaseRenderPass(InShaderName)
{
}

FDepthPrePass::~FDepthPrePass()
{
}

void FDepthPrePass::AddRenderObjectsToRenderPass(UWorld* InWorld)
{
    for (const AActor* actor : InWorld->GetActors())
    {
        for (const UActorComponent* actorComp : actor->GetComponents())
        {
            if (UStaticMeshComponent* pStaticMeshComp = Cast<UStaticMeshComponent>(actorComp))
            {
                if (!Cast<UGizmoBaseComponent>(actorComp))
                    StaticMesheComponents.Add(pStaticMeshComp);
            }
        }
    }
}

void FDepthPrePass::Prepare(std::shared_ptr<FViewportClient> InViewportClient)
{
    FBaseRenderPass::Prepare(InViewportClient);
    const FRenderer& Renderer = GEngine->renderer;
    const FGraphicsDevice& Graphics = GEngine->graphicDevice;
    FRenderResourceManager* renderResourceManager = Renderer.GetResourceManager();
    Graphics.DeviceContext->PSSetShader(nullptr, nullptr, 0);

    Graphics.DeviceContext->OMSetDepthStencilState(Renderer.GetDepthStencilState(EDepthStencilState::LessEqual), 0);
    Graphics.DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // 정정 연결 방식 설정
    Graphics.DeviceContext->RSSetState(renderResourceManager->GetRasterizerState(ERasterizerState::SolidBack));

    Graphics.DeviceContext->ClearDepthStencilView(Graphics.DepthPrePassDSV, D3D11_CLEAR_DEPTH,1, 0);
    Graphics.DeviceContext->OMSetRenderTargets(0, nullptr, Graphics.DepthPrePassDSV);
    Graphics.DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff); // 블렌딩 상태 설정, 기본 블렌딩 상태임
}

void FDepthPrePass::Execute(std::shared_ptr<FViewportClient> InViewportClient)
{
    FRenderer& Renderer = GEngine->renderer;
    FGraphicsDevice& Graphics = GEngine->graphicDevice;
    
    FMatrix View = FMatrix::Identity;
    FMatrix Proj = FMatrix::Identity;
    
    std::shared_ptr<FEditorViewportClient> curEditorViewportClient = std::dynamic_pointer_cast<FEditorViewportClient>(InViewportClient);
    if (curEditorViewportClient != nullptr)
    {
        View = curEditorViewportClient->GetViewMatrix();
        Proj = curEditorViewportClient->GetProjectionMatrix();
    }
    
    for (UStaticMeshComponent* staticMeshComp : StaticMesheComponents)
    {
        const FMatrix Model = JungleMath::CreateModelMatrix(staticMeshComp->GetComponentLocation(), staticMeshComp->GetComponentRotation(),
                                                    staticMeshComp->GetComponentScale());
        
        FVector4 UUIDColor = staticMeshComp->EncodeUUID() / 255.0f ;
        uint32 isSelected = 0;
        if (GEngine->GetWorld()->GetSelectedActors().Contains(staticMeshComp->GetOwner()))
        {
            isSelected = 1;
        }

        if (!staticMeshComp->GetStaticMesh()) continue;
        
        const OBJ::FStaticMeshRenderData* renderData = staticMeshComp->GetStaticMesh()->GetRenderData();
        if (renderData == nullptr) continue;

        // VIBuffer Bind
        const std::shared_ptr<FVBIBTopologyMapping> VBIBTopMappingInfo = Renderer.GetVBIBTopologyMapping(staticMeshComp->GetVBIBTopologyMappingName());
        VBIBTopMappingInfo->Bind();

        // If There's No Material Subset
        if (renderData->MaterialSubsets.Num() == 0)
        {
            Graphics.DeviceContext->DrawIndexed(VBIBTopMappingInfo->GetNumIndices(), 0,0);
        }

        // SubSet마다 Material Update 및 Draw
        for (int subMeshIndex = 0; subMeshIndex < renderData->MaterialSubsets.Num(); ++subMeshIndex)
        {
            const int materialIndex = renderData->MaterialSubsets[subMeshIndex].MaterialIndex;
            
            // index draw
            const uint64 startIndex = renderData->MaterialSubsets[subMeshIndex].IndexStart;
            const uint64 indexCount = renderData->MaterialSubsets[subMeshIndex].IndexCount;
            Graphics.DeviceContext->DrawIndexed(indexCount, startIndex, 0);
        }
    }
}

void FDepthPrePass::ClearRenderObjects()
{
    StaticMesheComponents.Empty();
    FBaseRenderPass::ClearRenderObjects();
}

void FDepthPrePass::UpdateMatrixConstants(UStaticMeshComponent* InStaticMeshComponent, const FMatrix& InView, const FMatrix& InProjection)
{
    FRenderResourceManager* renderResourceManager = GEngine->renderer.GetResourceManager();
    // MVP Update
    const FMatrix Model = JungleMath::CreateModelMatrix(InStaticMeshComponent->GetComponentLocation(), InStaticMeshComponent->GetComponentRotation(),
                                                        InStaticMeshComponent->GetComponentScale());
    const FMatrix NormalMatrix = FMatrix::Transpose(FMatrix::Inverse(Model));
        
    FMatrixConstants MatrixConstants;
    MatrixConstants.Model = Model;
    MatrixConstants.ViewProj = InView * InProjection;
    MatrixConstants.MInverseTranspose = NormalMatrix;
    if (InStaticMeshComponent->GetWorld()->GetSelectedActors().Contains(InStaticMeshComponent->GetOwner()))
    {
        MatrixConstants.isSelected = true;
    }
    else
    {
        MatrixConstants.isSelected = false;
    }
    renderResourceManager->UpdateConstantBuffer(TEXT("FMatrixConstants"), &MatrixConstants);
}
