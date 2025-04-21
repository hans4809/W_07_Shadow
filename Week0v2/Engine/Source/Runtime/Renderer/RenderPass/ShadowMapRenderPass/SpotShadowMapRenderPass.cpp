#include "SpotShadowMapRenderPass.h"
#include "Engine/LightManager.h"
#include "EditorEngine.h"
#include "Components/Mesh/StaticMesh.h"
#include "Math/JungleMath.h"
#include "Components/PrimitiveComponents/MeshComponents/StaticMeshComponents/StaticMeshComponent.h"
#include "Renderer/VBIBTopologyMapping.h"
#include <d3dcompiler.h>
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

    renderResourceManager->AddOrSetSRVStructuredBuffer(SpotLightVPMat, SB);
    renderResourceManager->AddOrSetSRVStructuredBufferSRV(SpotLightVPMat, SBSRV);

    CreateShadowMapResource();

    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_BACK;
    rasterDesc.DepthClipEnable = true;

    // Depth Bias 설정
    rasterDesc.DepthBias = 1;            // 정수 값
    rasterDesc.SlopeScaledDepthBias = 1.0f;       // 기울기 기반 Bias
    rasterDesc.DepthBiasClamp = 0.0f;             // Bias 최대값 제한 (보통 0.0f)

    // Rasterizer State 생성
    HRESULT hr = Graphics.Device->CreateRasterizerState(&rasterDesc, &rasterizerState);
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

    D3D11_VIEWPORT shadowViewport = {};
    shadowViewport.TopLeftX = 0;
    shadowViewport.TopLeftY = 0;
    shadowViewport.Width = (FLOAT)MapWidth;
    shadowViewport.Height = (FLOAT)MapHeight;
    shadowViewport.MinDepth = 0.0f;
    shadowViewport.MaxDepth = 1.0f;

    Graphics.DeviceContext->RSSetViewports(1, &shadowViewport);

    ID3D11ShaderResourceView* SBSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("SpotLightVPMat"));
    Graphics.DeviceContext->VSSetShaderResources(0, 1, &SBSRV);

    Graphics.DeviceContext->PSSetShader(nullptr, nullptr, 0);

    ID3D11DepthStencilState* DepthStencilState =
        Renderer.GetDepthStencilState(EDepthStencilState::LessEqual);
    Graphics.DeviceContext->OMSetDepthStencilState(DepthStencilState, 0);
    
    ID3D11DepthStencilView* ShadowMapDSVArray =
        renderResourceManager->GetShadowMapDSV(ShadowMap);
    Graphics.DeviceContext->ClearDepthStencilView(ShadowMapDSVArray, D3D11_CLEAR_DEPTH, 1, 0);
    Graphics.DeviceContext->OMSetRenderTargets(0, nullptr, ShadowMapDSVArray);

    Graphics.DeviceContext->RSSetState(rasterizerState);
}

void FSpotShadowMapRenderPass::Execute(std::shared_ptr<FViewportClient> InViewportClient)
{
    FShadowMapRenderPass::Execute(InViewportClient);

    FRenderer& Renderer = GEngine->renderer;
    FGraphicsDevice& Graphics = GEngine->graphicDevice;

    FLightManager* LightManager = Renderer.LightManager;
    uint32 NumSpotLights = LightManager->GetVisibleSpotLights().Num();
    UpdateLightStructuredBuffer();
    for (const UStaticMeshComponent* staticMeshComp : StaticMeshComponents)
    {
        const FMatrix Model = JungleMath::CreateModelMatrix(staticMeshComp->GetComponentLocation(), staticMeshComp->GetComponentRotation(),
            staticMeshComp->GetComponentScale());

        FSpotCB spotCB;
        spotCB.ModelMatrix = Model;
        spotCB.NumSpotLights = NumSpotLights;

        Renderer.GetResourceManager()->UpdateConstantBuffer(TEXT("FSpotCB"), &spotCB);
        
        if (!staticMeshComp->GetStaticMesh()) continue;
        
        const OBJ::FStaticMeshRenderData* renderData = staticMeshComp->GetStaticMesh()->GetRenderData();
        if (!renderData) continue;

        const std::shared_ptr<FVBIBTopologyMapping> VBIBTopMappingInfo = Renderer.GetVBIBTopologyMapping(staticMeshComp->GetVBIBTopologyMappingName());
        VBIBTopMappingInfo->Bind();

        // If There's No Material Subset
        if (renderData->MaterialSubsets.Num() == 0)
        {
            Graphics.DeviceContext->DrawIndexedInstanced(VBIBTopMappingInfo->GetNumIndices(), NumSpotLights, 0, 0, 0);
        }

        // SubSet마다 Material Update 및 Draw
        for (int subMeshIndex = 0; subMeshIndex < renderData->MaterialSubsets.Num(); ++subMeshIndex)
        {
            const int materialIndex = renderData->MaterialSubsets[subMeshIndex].MaterialIndex;

            // index draw
            const uint64 startIndex = renderData->MaterialSubsets[subMeshIndex].IndexStart;
            const uint64 indexCount = renderData->MaterialSubsets[subMeshIndex].IndexCount;
            Graphics.DeviceContext->DrawIndexedInstanced(indexCount, NumSpotLights, startIndex, 0, 0);
        }
    }

    Graphics.DeviceContext->OMSetRenderTargets(0, nullptr, nullptr); // 완전히 언바인드
}

void FSpotShadowMapRenderPass::ClearRenderObjects()
{
    FShadowMapRenderPass::ClearRenderObjects();
}

void FSpotShadowMapRenderPass::CreateShadowMapResource()
{
    FRenderer& Renderer = GEngine->renderer;
    FRenderResourceManager* renderResourceManager = Renderer.GetResourceManager();
    
    ID3D11Texture2D* ShadowMapTexture2DArray =
        renderResourceManager->CreateTexture2DArray(MapWidth, MapHeight, MAX_SPOT_LIGHTS);
    ID3D11DepthStencilView* ShadowMapDSVArray =
        renderResourceManager->CreateTexture2DArrayDSV(ShadowMapTexture2DArray, MAX_SPOT_LIGHTS);
    ID3D11ShaderResourceView* ShadowMapSRVArray =
        renderResourceManager->CreateTexture2DArraySRV(ShadowMapTexture2DArray, MAX_SPOT_LIGHTS);
    TArray<ID3D11ShaderResourceView*> Texture2DArraySliceSRVs = 
        renderResourceManager->CreateTexture2DArraySliceSRVs(ShadowMapTexture2DArray, MAX_SPOT_LIGHTS);

    renderResourceManager->AddOrSetSRVShadowMapTexutre(ShadowMap, ShadowMapTexture2DArray);
    renderResourceManager->AddOrSetSRVShadowMapSRV(ShadowMap, ShadowMapSRVArray);
    renderResourceManager->AddOrSetDSVShadowMapTexutre(ShadowMap, ShadowMapTexture2DArray);
    renderResourceManager->AddOrSetDSVShadowMapDSV(ShadowMap, ShadowMapDSVArray);
    renderResourceManager->AddOrSetSRVShadowMapSlice(ShadowMap, Texture2DArraySliceSRVs);
}

void FSpotShadowMapRenderPass::UpdateLightStructuredBuffer()
{
    FRenderer& Renderer = GEngine->renderer;
    FRenderResourceManager* renderResourceManager = Renderer.GetResourceManager();

    ID3D11Buffer* SB = renderResourceManager->GetSRVStructuredBuffer(TEXT("SpotLightVPMat"));

    FLightManager* LightManager = Renderer.LightManager;
    TArray<USpotLightComponent*> VisibleSpotLights = LightManager->GetVisibleSpotLights();

    TArray<FLightVP> SpotLightViewProjMatrices;

    for (USpotLightComponent* LightComp : VisibleSpotLights)
    {
        if (!LightComp) continue;

        FLightVP GPULight;
        GPULight.LightVP = ComputeViewProj(LightComp);

        SpotLightViewProjMatrices.Add(GPULight);
    }

    const FGraphicsDevice& Graphics = GEngine->graphicDevice;

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = Graphics.DeviceContext->Map(SB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

    if (hr >= 0)
    {
        memcpy(mapped.pData, SpotLightViewProjMatrices.GetData(), sizeof(FLightVP) * SpotLightViewProjMatrices.Num());
        Graphics.DeviceContext->Unmap(SB, 0);
    }
}

FMatrix FSpotShadowMapRenderPass::ComputeViewProj(const USpotLightComponent* LightComp)
{
    const FVector LightPos = LightComp->GetComponentLocation();
    const FVector LightDir = LightComp->GetOwner()->GetActorForwardVector();
    const FVector LightUp = LightComp->GetOwner()->GetActorUpVector();

    const FMatrix ViewMatrix =
        JungleMath::CreateViewMatrix(LightPos, LightPos + LightDir, LightUp);

    const float OuterConeAngle = LightComp->GetOuterConeAngle();
    const float AspectRatio = 1.0f;
    const float NearZ = 1.0f;
    const float FarZ = LightComp->GetRadius();

    const FMatrix ProjectionMatrix =
        JungleMath::CreateProjectionMatrix(OuterConeAngle*2, AspectRatio, NearZ, FarZ);

    return ViewMatrix * ProjectionMatrix;
}
