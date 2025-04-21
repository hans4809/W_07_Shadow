#include "SpotShadowMapRenderPass.h"

#include "EditorEngine.h"
#include "LightManager.h"
#include "Components/Mesh/StaticMesh.h"
#include "Components/PrimitiveComponents/MeshComponents/StaticMeshComponents/StaticMeshComponent.h"
#include "D3D11RHI/ShadowMapConfig.h"
#include "Math/JungleMath.h"
#include "Renderer/VBIBTopologyMapping.h"
#include "UnrealEd/EditorViewportClient.h"

FSpotShadowMapRenderPass::FSpotShadowMapRenderPass(const FName& InShaderName)
    : FShadowMapRenderPass(InShaderName)
{
    FRenderer& Renderer = GEngine->renderer;
    FRenderResourceManager* renderResourceManager = Renderer.GetResourceManager();
    const FGraphicsDevice& Graphics = GEngine->graphicDevice;
    
    ID3D11Buffer* SB = nullptr;
    ID3D11ShaderResourceView* SBSRV = nullptr;
    SB = renderResourceManager->CreateStructuredBuffer<FMatrix>(MAX_SPOT_LIGHTS);
    SBSRV = renderResourceManager->CreateBufferSRV(SB, MAX_SPOT_LIGHTS);

    renderResourceManager->AddOrSetSRVStructuredBuffer(TEXT("SpotLightVPMat"), SB);
    renderResourceManager->AddOrSetSRVStructuredBufferSRV(TEXT("SpotLightVPMat"), SBSRV);
}
 
FSpotShadowMapRenderPass::~FSpotShadowMapRenderPass()
{
}

void FSpotShadowMapRenderPass::AddRenderObjectsToRenderPass(UWorld* InLevel)
{
    FShadowMapRenderPass::AddRenderObjectsToRenderPass(InLevel);
    // TODO : SpotLightComp 추가
}

void FSpotShadowMapRenderPass::Prepare(std::shared_ptr<FViewportClient> InViewportClient)
{
    FShadowMapRenderPass::Prepare(InViewportClient);

    FRenderer& Renderer = GEngine->renderer;
    FRenderResourceManager* renderResourceManager = Renderer.GetResourceManager();
    const FGraphicsDevice& Graphics = GEngine->graphicDevice;

    Graphics.DeviceContext->PSSetShader(nullptr, nullptr, 0);

    ID3D11ShaderResourceView* SBSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("SpotLightVPMat"));
    Graphics.DeviceContext->VSSetShaderResources(0, 1, &SBSRV);
    
    Graphics.DeviceContext->OMSetDepthStencilState(Renderer.GetDepthStencilState(EDepthStencilState::LessEqual), 0);
    Graphics.DeviceContext->ClearDepthStencilView(Graphics.SpotShadowDSV, D3D11_CLEAR_DEPTH,1,0);
    Graphics.DeviceContext->OMSetRenderTargets(0, nullptr, Graphics.SpotShadowDSV);
} 

void FSpotShadowMapRenderPass::Execute(std::shared_ptr<FViewportClient> InViewportClient)
{
    FShadowMapRenderPass::Execute(InViewportClient);
    const FGraphicsDevice& Graphics = GEngine->graphicDevice;
    FRenderer& Renderer = GEngine->renderer;
    FLightManager* lightManager = Renderer.LightManager;
    FRenderResourceManager* renderResourceManager = Renderer.GetResourceManager();
    
    std::shared_ptr<FEditorViewportClient> curEditorViewportClient = std::dynamic_pointer_cast<FEditorViewportClient>(InViewportClient);
    const float camNear = curEditorViewportClient->nearPlane;
    const float camFar = curEditorViewportClient->farPlane;

    TArray<FMatrix> viewProjMatrixes;
    for (uint32 visibleLightIndex = 0; visibleLightIndex < lightManager->GetVisibleSpotLights().Num(); ++visibleLightIndex)
    {
        USpotLightComponent* spotLight = lightManager->GetVisibleSpotLights()[visibleLightIndex];
        FVector LightPos = spotLight->GetOwner()->GetActorLocation();
        FVector LightTarget = LightPos + spotLight->GetOwner()->GetActorForwardVector();
        FVector LightUp = spotLight->GetOwner()->GetActorUpVector();
        FMatrix LightView = JungleMath::CreateViewMatrix(LightPos, LightTarget, LightUp);
        FMatrix LightProjection = JungleMath::CreateProjectionMatrix(spotLight->GetOuterConeRad() * 2.f, 1.0f, camNear, camFar);
        FMatrix VP = LightView * LightProjection;
        viewProjMatrixes.Add(VP);
    }
    renderResourceManager->UpdateStructuredBuffer(TEXT("SpotLightVPMat"), viewProjMatrixes);

    for (uint32 visibleLightIndex = 0; visibleLightIndex < lightManager->GetVisibleSpotLights().Num(); ++visibleLightIndex)
    {
        const uint32 col = visibleLightIndex % Shadow::DIR_ATLAS_COLS;
        const uint32 row = visibleLightIndex / Shadow::DIR_ATLAS_ROWS;
        D3D11_VIEWPORT viewport =
        {
            static_cast<float>(col * Shadow::SPOT_LIGHT_RES), static_cast<float>(row * Shadow::SPOT_LIGHT_RES),
            Shadow::SPOT_LIGHT_RES, Shadow::SPOT_LIGHT_RES,
            0.0f, 1.0f,
        };
        Graphics.DeviceContext->RSSetViewports(1, &viewport);

        for (const UStaticMeshComponent* staticMeshComp : StaticMeshComponents)
        {
            FSpotCB spotCB;
            const FMatrix Model = JungleMath::CreateModelMatrix(staticMeshComp->GetComponentLocation(), staticMeshComp->GetComponentRotation(),
                                                        staticMeshComp->GetComponentScale());
            spotCB.ModelMatrix = Model;
            spotCB.SpotIndex = visibleLightIndex;

            Renderer.GetResourceManager()->UpdateConstantBuffer(TEXT("FSpotCB"), &spotCB);

            if (!staticMeshComp->GetStaticMesh()) continue;
        
            const OBJ::FStaticMeshRenderData* renderData = staticMeshComp->GetStaticMesh()->GetRenderData();
            if (renderData == nullptr) continue;

            // VIBuffer Bind
            const std::shared_ptr<FVBIBTopologyMapping> VBIBTopMappingInfo = Renderer.GetVBIBTopologyMapping(staticMeshComp->GetVBIBTopologyMappingName());
            VBIBTopMappingInfo->Bind();

            // If There's No Material Subset
            if (renderData->MaterialSubsets.Num() == 0)
            {
                Graphics.DeviceContext->DrawIndexed(VBIBTopMappingInfo->GetNumIndices(), 0, 0);
            }

            // SubSet마다 Material Update 및 Draw
            for (int subMeshIndex = 0; subMeshIndex < renderData->MaterialSubsets.Num(); ++subMeshIndex)
            {
                // index draw
                const uint64 startIndex = renderData->MaterialSubsets[subMeshIndex].IndexStart;
                const uint64 indexCount = renderData->MaterialSubsets[subMeshIndex].IndexCount;
                Graphics.DeviceContext->DrawIndexed(indexCount, startIndex, 0);
            }
        }
    }
}

void FSpotShadowMapRenderPass::ClearRenderObjects()
{
    FShadowMapRenderPass::ClearRenderObjects();
}
