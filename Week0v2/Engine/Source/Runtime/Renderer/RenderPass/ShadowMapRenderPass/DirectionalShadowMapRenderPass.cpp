#include "DirectionalShadowMapRenderPass.h"

#include "EditorEngine.h"
#include "LightManager.h"
#include "Components/Mesh/StaticMesh.h"
#include "Components/PrimitiveComponents/MeshComponents/StaticMeshComponents/StaticMeshComponent.h"
#include "D3D11RHI/ShadowMapConfig.h"
#include "Math/Color.h"
#include "Math/JungleMath.h"
#include "Renderer/VBIBTopologyMapping.h"
#include "ShaderHeaders/GConstantBuffers.hlsli"
#include "UnrealEd/EditorViewportClient.h"

FDirectionalShadowMapRenderPass::FDirectionalShadowMapRenderPass(const FName& InShaderName)
    : FShadowMapRenderPass(InShaderName)
{
}

FDirectionalShadowMapRenderPass::~FDirectionalShadowMapRenderPass()
{
}

void FDirectionalShadowMapRenderPass::AddRenderObjectsToRenderPass(UWorld* InLevel)
{
    FShadowMapRenderPass::AddRenderObjectsToRenderPass(InLevel);
}

void FDirectionalShadowMapRenderPass::Prepare(std::shared_ptr<FViewportClient> InViewportClient)
{
    FShadowMapRenderPass::Prepare(InViewportClient);
    FRenderer& Renderer = GEngine->renderer;
    FGraphicsDevice& Graphics = GEngine->graphicDevice;
    //Graphics.DeviceContext->PSSetShader(nullptr, nullptr, 0);

    ID3D11DepthStencilState* ds = Renderer.GetDepthStencilState(EDepthStencilState::LessEqual);
    Graphics.DeviceContext->OMSetDepthStencilState(Renderer.GetDepthStencilState(EDepthStencilState::LessEqual), 0);
    Graphics.DeviceContext->ClearRenderTargetView(Graphics.DirFrameBufferRTV, Graphics.ClearColor);
    Graphics.DeviceContext->ClearDepthStencilView(Graphics.DirShadowDSV, D3D11_CLEAR_DEPTH,1,0);
    Graphics.DeviceContext->OMSetRenderTargets(1, &Graphics.DirFrameBufferRTV, Graphics.DirShadowDSV);
}

void FDirectionalShadowMapRenderPass::Execute(std::shared_ptr<FViewportClient> InViewportClient)
{
    FShadowMapRenderPass::Execute(InViewportClient);
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
    

    const float camNear = curEditorViewportClient->nearPlane;
    const float camFar = curEditorViewportClient->farPlane;
    float cascadeSplits[Shadow::CASCADE_COUNT + 1];
    cascadeSplits[0]                = camNear;
    cascadeSplits[Shadow::CASCADE_COUNT]     = camFar;

    const float lambda = 0.5f; // 0=완전 균등, 1=완전 로그, 0.5=절충
    const float minCascadeDepth = 1.0f; // 또는 0.5f, 원하는 값으로 조정

    for(int i = 1; i < Shadow::CASCADE_COUNT; ++i)
    {
        const float si        = static_cast<float>(i) / Shadow::CASCADE_COUNT;       // 0..1
        // 1) 균등 분할
        const float uniform = camNear + (camFar - camNear) * si;
        // 2) 로그 분할
        const float logSplit = camNear * powf(camFar / camNear, si);
        // 3) 절충
        float splitZ = uniform * (1 - lambda) + logSplit * lambda;

        //  너무 얕은 구간은 강제로 보정
        cascadeSplits[i] = FMath::Max(splitZ, minCascadeDepth);
    }
    
    for (uint32 cascade = 0; cascade < Shadow::CASCADE_COUNT; ++cascade)
    {
        FCascadeCB cascadeCB;
        FMatrix lightView, lightProj;
        JungleMath::ComputeDirLightVP(
            Renderer.LightManager->GetDirectionalLight()->GetForwardVector(),
            View, Proj,
            cascadeSplits[cascade], cascadeSplits[cascade+1],
            lightView, lightProj
        );
        cascadeCB.LightVP = lightView * lightProj;

        const uint32 col = cascade % Shadow::ATLAS_COLS;
        const uint32 row = cascade / Shadow::ATLAS_COLS;
        D3D11_VIEWPORT viewport =
        {
            static_cast<float>(col * Shadow::CASCADE_RES), static_cast<float>(row * Shadow::CASCADE_RES),
            Shadow::CASCADE_RES, Shadow::CASCADE_RES,
            0.0f, 1.0f,
        };
        Graphics.DeviceContext->RSSetViewports(1, &viewport);

        for (const UStaticMeshComponent* staticMeshComp : StaticMeshComponents)
        {
            const FMatrix Model = JungleMath::CreateModelMatrix(staticMeshComp->GetComponentLocation(), staticMeshComp->GetComponentRotation(),
                                                        staticMeshComp->GetComponentScale());
            cascadeCB.ModelMatrix = Model;

            Renderer.GetResourceManager()->UpdateConstantBuffer(TEXT("FCascadeCB"), &cascadeCB);

            if (!staticMeshComp->GetStaticMesh()) continue;
        
            const OBJ::FStaticMeshRenderData* renderData = staticMeshComp->GetStaticMesh()->GetRenderData();
            if (renderData == nullptr) continue;

            // VIBuffer Bind
            const std::shared_ptr<FVBIBTopologyMapping> VBIBTopMappingInfo = Renderer.GetVBIBTopologyMapping(staticMeshComp->GetVBIBTopologyMappingName());
            VBIBTopMappingInfo->Bind();

            // If There's No Material Subset
            if (renderData->MaterialSubsets.Num() == 0)
            {
                Graphics.DeviceContext->DrawIndexedInstanced(VBIBTopMappingInfo->GetNumIndices(), MAX_CASCADES, 0, 0, 0);
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
}

void FDirectionalShadowMapRenderPass::ClearRenderObjects()
{
    FShadowMapRenderPass::ClearRenderObjects();
}
