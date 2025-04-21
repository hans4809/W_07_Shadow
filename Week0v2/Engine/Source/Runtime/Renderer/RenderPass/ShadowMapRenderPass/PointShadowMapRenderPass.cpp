#include "PointShadowMapRenderPass.h"

#include "EditorEngine.h"
#include "LightManager.h"
#include "Components/Mesh/StaticMesh.h"
#include "Components/PrimitiveComponents/MeshComponents/StaticMeshComponents/StaticMeshComponent.h"
#include "D3D11RHI/ShadowMapConfig.h"
#include "Math/JungleMath.h"
#include "Renderer/VBIBTopologyMapping.h"
#include "UnrealEd/EditorViewportClient.h"

FPointShadowMapRenderPass::FPointShadowMapRenderPass(const FName& InShaderName)
    : FShadowMapRenderPass(InShaderName)
{
    FRenderer& Renderer = GEngine->renderer;
    FRenderResourceManager* renderResourceManager = Renderer.GetResourceManager();
    const FGraphicsDevice& Graphics = GEngine->graphicDevice;
    
    ID3D11Buffer* SB = nullptr;
    ID3D11ShaderResourceView* SBSRV = nullptr;
    SB = renderResourceManager->CreateStructuredBuffer<FPointLightVP>(MAX_POINT_LIGHTS);
    SBSRV = renderResourceManager->CreateBufferSRV(SB, MAX_POINT_LIGHTS);

    renderResourceManager->AddOrSetSRVStructuredBuffer(TEXT("PointLightVPMat"), SB);
    renderResourceManager->AddOrSetSRVStructuredBufferSRV(TEXT("PointLightVPMat"), SBSRV);
}

FPointShadowMapRenderPass::~FPointShadowMapRenderPass()
{
}

void FPointShadowMapRenderPass::AddRenderObjectsToRenderPass(UWorld* InLevel)
{
    FShadowMapRenderPass::AddRenderObjectsToRenderPass(InLevel);
}

void FPointShadowMapRenderPass::Prepare(std::shared_ptr<FViewportClient> InViewportClient)
{
    FShadowMapRenderPass::Prepare(InViewportClient);

    FRenderer& Renderer = GEngine->renderer;
    FRenderResourceManager* renderResourceManager = Renderer.GetResourceManager();
    const FGraphicsDevice& Graphics = GEngine->graphicDevice;

    Graphics.DeviceContext->PSSetShader(nullptr, nullptr, 0);
    
    ID3D11ShaderResourceView* SBSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("PointLightVPMat"));
    Graphics.DeviceContext->VSSetShaderResources(0, 1, &SBSRV);

    Graphics.DeviceContext->OMSetDepthStencilState(Renderer.GetDepthStencilState(EDepthStencilState::LessEqual), 0);
    Graphics.DeviceContext->ClearDepthStencilView(Graphics.PointShadowDSV, D3D11_CLEAR_DEPTH,1,0);
    Graphics.DeviceContext->OMSetRenderTargets(0, nullptr, Graphics.PointShadowDSV);

    Graphics.DeviceContext->GSSetShaderResources(0, 1, &SBSRV);
}

void FPointShadowMapRenderPass::Execute(std::shared_ptr<FViewportClient> InViewportClient)
{
    FShadowMapRenderPass::Execute(InViewportClient);

    const FGraphicsDevice& Graphics = GEngine->graphicDevice;
    FRenderer& Renderer = GEngine->renderer;
    FLightManager* lightManager = Renderer.LightManager;
    FRenderResourceManager* renderResourceManager = Renderer.GetResourceManager();

    std::shared_ptr<FEditorViewportClient> curEditorViewportClient = std::dynamic_pointer_cast<FEditorViewportClient>(InViewportClient);
    const float camNear = curEditorViewportClient->nearPlane;
    const float camFar = curEditorViewportClient->farPlane;

    TArray<FPointLightVP> viewProjMatrixes;
    for (uint32 visibleLightIndex = 0; visibleLightIndex < lightManager->GetVisiblePointLights().Num(); ++visibleLightIndex)
    {
        const UPointLightComponent* pointLight = lightManager->GetVisiblePointLights()[visibleLightIndex];
        FVector LightPos = pointLight->GetOwner()->GetActorLocation();
        FPointLightVP PointVPs;
        for (uint32 face = 0; face < 6; ++face)
        {
            static const FVector ForwardVector[6] = {
                {1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}
            };
            static const FVector UpVector[6] = {
                {0,1,0},{0,1,0},{0,0,-1},{0,0,1},{0,1,0},{0,1,0}
            };

            const FVector LightTarget = LightPos + ForwardVector[face];
            const FVector LightUp = UpVector[face];
            FMatrix LightView = JungleMath::CreateViewMatrix(LightPos, LightTarget, LightUp);

            FMatrix LightProjection = JungleMath::CreateProjectionMatrix(PIDIV2, 1.0f, camNear, camFar);
            const FMatrix VP = LightView * LightProjection;
            PointVPs.VP[face] = VP;
        }
        viewProjMatrixes.Add(PointVPs);
    }
    renderResourceManager->UpdateStructuredBuffer(TEXT("PointLightVPMat"), viewProjMatrixes);
    
    for (uint32 visibleLightIndex = 0; visibleLightIndex < lightManager->GetVisiblePointLights().Num(); ++visibleLightIndex)
    {
        D3D11_VIEWPORT vp[6] = {};
        for (uint32 face = 0; face < 6; ++face)
        {
            const uint32 row = visibleLightIndex % Shadow::POINT_ATLAS_ROWS;
            const uint32 col = face % Shadow::POINT_ATLAS_COLS;

            vp[face] = 
            {
                static_cast<float>(col * Shadow::POINT_LIGHT_RES),
                static_cast<float>(row * Shadow::POINT_LIGHT_RES),
                static_cast<float>(Shadow::POINT_LIGHT_RES),
                static_cast<float>(Shadow::POINT_LIGHT_RES),
                0.0f, 1.0f
            };
        }
        Graphics.DeviceContext->RSSetViewports(6, vp);

        for (const UStaticMeshComponent* staticMeshComp : StaticMeshComponents)
        {
            FPointCB spotCB;
            const FMatrix Model = JungleMath::CreateModelMatrix(staticMeshComp->GetComponentLocation(), staticMeshComp->GetComponentRotation(),
                                                        staticMeshComp->GetComponentScale());
            spotCB.ModelMatrix = Model;
            spotCB.PointIndex = visibleLightIndex;

            Renderer.GetResourceManager()->UpdateConstantBuffer(TEXT("FPointCB"), &spotCB);

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

void FPointShadowMapRenderPass::ClearRenderObjects()
{
    FShadowMapRenderPass::ClearRenderObjects();
}
