#include "DirectionalShadowMapRenderPass.h"

#include "EditorEngine.h"
#include "LightManager.h"
#include "Components/Mesh/StaticMesh.h"
#include "Components/PrimitiveComponents/MeshComponents/StaticMeshComponents/StaticMeshComponent.h"
#include "Math/JungleMath.h"
#include "Renderer/VBIBTopologyMapping.h"
#include "ShaderHeaders/GConstantBuffers.hlsli"
#include "UnrealEd/EditorViewportClient.h"

FDirectionalShadowMapRenderPass::FDirectionalShadowMapRenderPass(const FName& InShaderName)
    : FShadowMapRenderPass(InShaderName)
{
    CreateShadowMapResource();
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

    FGraphicsDevice& Graphics = GEngine->graphicDevice;
    FRenderer& Renderer = GEngine->renderer;
    FRenderResourceManager* renderResourceManager = Renderer.GetResourceManager();

    auto curConstantBuffer = Renderer.GetResourceManager()->GetConstantBuffer(TEXT("FCascadeCB"));

    Graphics.DeviceContext->GSSetConstantBuffers(0, 1, &curConstantBuffer);
    Graphics.DeviceContext->PSSetShader(nullptr, nullptr, 0);

    ID3D11DepthStencilState* DepthStencilState =
            Renderer.GetResourceManager()->GetDepthStencilState(EDepthStencilState::LessEqual);
    Graphics.DeviceContext->OMSetDepthStencilState(DepthStencilState, 0);

    ID3D11DepthStencilView* ShadowMapDSVArray =
        renderResourceManager->GetShadowMapDSV(DirLightShadowMap);
    Graphics.DeviceContext->ClearDepthStencilView(ShadowMapDSVArray, D3D11_CLEAR_DEPTH,1,0);
    Graphics.DeviceContext->OMSetRenderTargets(0, nullptr, ShadowMapDSVArray);

    D3D11_VIEWPORT vp =
    {
        0, 0,
        static_cast<float>(MapWidth),
        static_cast<float>(MapHeight),
        0.0f, 1.0f
    };

    Graphics.DeviceContext->RSSetViewports(1, &vp);
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
    
    FCascadeCB cascadeCB;
    const float camNear = curEditorViewportClient->nearPlane;
    const float camFar = curEditorViewportClient->farPlane;
    float cascadeSplits[MAX_CASCADES + 1];
    cascadeSplits[0]                = camNear;
    cascadeSplits[MAX_CASCADES]     = camFar;

    const float lambda = 1.0f; // 0=완전 균등, 1=완전 로그, 0.5=절충
    const float minCascadeDepth = 1.0f; // 또는 0.5f, 원하는 값으로 조정

    for (int i = 1; i < MAX_CASCADES; ++i)
    {
        const float si = static_cast<float>(i) / MAX_CASCADES;       // 0..1
        // 1) 균등 분할
        const float uniform = camNear + (camFar - camNear) * si;
        // 2) 로그 분할
        const float logSplit = camNear * powf(camFar / camNear, si);
        // 3) 절충
        float splitZ = uniform * (1 - lambda) + logSplit * lambda;

        //  너무 얕은 구간은 강제로 보정
        cascadeSplits[i] = FMath::Max(splitZ, minCascadeDepth);
    }
    Renderer.LightManager->GetDirectionalLight()->SetCascadeSplits(cascadeSplits);
    cascadeCB.NumCascades = MAX_CASCADES;
    for (int i = 0; i < MAX_CASCADES; ++i)
    {
        FMatrix lightView, lightProj;
        JungleMath::ComputeDirLightVP(
            Renderer.LightManager->GetDirectionalLight()->GetOwner()->GetActorForwardVector(),
            View, Proj,
            cascadeSplits[i], cascadeSplits[i+1],
            camNear, camFar,
            lightView, lightProj
        );
        cascadeCB.LightVP[i] = lightView * lightProj;
        Renderer.LightManager->GetDirectionalLight()->SetViewProjectionMatrix(i, cascadeCB.LightVP[i]);
    }

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
            Graphics.DeviceContext->DrawIndexedInstanced(indexCount, MAX_CASCADES, startIndex, 0, 0);
        }
    }
    
    // 바인딩 해제
    Graphics.DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}

void FDirectionalShadowMapRenderPass::ClearRenderObjects()
{
    FShadowMapRenderPass::ClearRenderObjects();
}

void FDirectionalShadowMapRenderPass::CreateShadowMapResource()
{
    FRenderer& Renderer = GEngine->renderer;
    FRenderResourceManager* renderResourceManager = Renderer.GetResourceManager();

    ID3D11Texture2D* ShadowMapTexture2DArray =
        renderResourceManager->CreateTexture2DArray(MapWidth, MapHeight, MAX_CASCADES);
    ID3D11DepthStencilView* ShadowMapDSVArray =
        renderResourceManager->CreateTexture2DArrayDSV(ShadowMapTexture2DArray, MAX_CASCADES);
    ID3D11ShaderResourceView* ShadowMapSRVArray =
        renderResourceManager->CreateTexture2DArraySRV(ShadowMapTexture2DArray, MAX_CASCADES);
    const TArray<ID3D11ShaderResourceView*> Texture2DArraySliceSRVs =
        renderResourceManager->CreateTexture2DArraySliceSRVs(ShadowMapTexture2DArray, MAX_CASCADES);

    renderResourceManager->AddOrSetShadowMapTexutre(DirLightShadowMap, ShadowMapTexture2DArray);
    renderResourceManager->AddOrSetShadowMapSRV(DirLightShadowMap, ShadowMapSRVArray);
    renderResourceManager->AddOrSetShadowMapDSV(DirLightShadowMap, ShadowMapDSVArray);
    renderResourceManager->AddOrSetSRVShadowMapSlice(DirLightShadowMap, Texture2DArraySliceSRVs);

}
