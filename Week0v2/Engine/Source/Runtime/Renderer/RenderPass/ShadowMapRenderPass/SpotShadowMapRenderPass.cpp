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
    //HRESULT hr = Graphics.Device->CreateRasterizerState(&rasterDesc, &rasterizerState);
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

    ID3D11ShaderResourceView* SBSRV = renderResourceManager->GetStructuredBufferSRV(SpotLightVPMat);
    Graphics.DeviceContext->VSSetShaderResources(0, 1, &SBSRV);

    Graphics.DeviceContext->PSSetShader(nullptr, nullptr, 0);

    ID3D11DepthStencilState* DepthStencilState =
        Renderer.GetDepthStencilState(EDepthStencilState::LessEqual);
    Graphics.DeviceContext->OMSetDepthStencilState(DepthStencilState, 0);
    
    ID3D11DepthStencilView* ShadowMapDSVArray =
        renderResourceManager->GetShadowMapDSV(SpotLightShadowMap);
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
    UpdateLightStructuredBuffer(InViewportClient);
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

    renderResourceManager->AddOrSetSRVShadowMapTexutre(SpotLightShadowMap, ShadowMapTexture2DArray);
    renderResourceManager->AddOrSetSRVShadowMapSRV(SpotLightShadowMap, ShadowMapSRVArray);
    renderResourceManager->AddOrSetDSVShadowMapTexutre(SpotLightShadowMap, ShadowMapTexture2DArray);
    renderResourceManager->AddOrSetDSVShadowMapDSV(SpotLightShadowMap, ShadowMapDSVArray);
    renderResourceManager->AddOrSetSRVShadowMapSlice(SpotLightShadowMap, Texture2DArraySliceSRVs);
}

void FSpotShadowMapRenderPass::UpdateLightStructuredBuffer(std::shared_ptr<FViewportClient> InViewportClient)
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
        GPULight.LightVP = ComputeViewProjPSM(LightComp,InViewportClient);
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
        JungleMath::CreateProjectionMatrix(OuterConeAngle * 2, AspectRatio, NearZ, FarZ);

    // Fix: Remove const qualifier to allow calling SetViewProjectionMatrix  
    const_cast<USpotLightComponent*>(LightComp)->SetViewMatrix(ViewMatrix);
    const_cast<USpotLightComponent*>(LightComp)->SetProjectionMatrix(ProjectionMatrix);

    return ViewMatrix * ProjectionMatrix;
}

FMatrix FSpotShadowMapRenderPass::ComputeViewProjPSM(const USpotLightComponent* LightComp, std::shared_ptr<FViewportClient> InViewportClient)
{  
    std::shared_ptr<FEditorViewportClient> Camera = std::dynamic_pointer_cast<FEditorViewportClient>(InViewportClient);

    FMatrix CameraView = Camera->GetViewMatrix();
    FMatrix CameraProj = Camera->GetProjectionMatrix();

    FMatrix CameraViewProj = CameraView * CameraProj;
    FMatrix InvCameraViewProj = FMatrix::Inverse(CameraViewProj);

    FVector NDCPoints[8] = {
    {-1,-1,0}, {1,-1,0}, {-1,1,0}, {1,1,0},
    {-1,-1,1}, {1,-1,1}, {-1,1,1}, {1,1,1},
    };

    FVector WorldPoints[8];
    for (int i = 0; i < 8; ++i)
    {
        FVector4 clip = FVector4(NDCPoints[i], 1);
        FVector4 world = InvCameraViewProj.TransformFVector4(clip);
        WorldPoints[i] = FVector(world.x/world.w,world.y/world.w,world.z/world.w);
    }

    FVector FrustumCenter = FVector(0,0,0);
    for (int i = 0; i < 8; ++i)
        FrustumCenter += WorldPoints[i];
    FrustumCenter /= 8.0f;

   const FVector LightUp = LightComp->GetOwner()->GetActorUpVector();  
   const FVector LightDir = LightComp->GetOwner()->GetActorForwardVector();  
   const FVector LightTarget = FrustumCenter;
   const FVector LightPos = LightTarget-LightDir*100.0f;  
   //const FVector LightPos = LightComp->GetComponentLocation();  
   const FMatrix ViewMatrix =   
        JungleMath::CreateViewMatrix(LightPos, LightPos+LightDir, LightUp);
   
   // 1. 라이트의 월드 위치를 카메라 ViewProj으로 Warp
   FVector4 LightWorld = FVector4(LightComp->GetComponentLocation(), 1.0f);
   FVector4 LightNDC = CameraViewProj.TransformFVector4(LightWorld);
   LightNDC = FVector4(LightNDC.x/LightNDC.w, LightNDC.y / LightNDC.w, LightNDC.z / LightNDC.w,1.0f);
   
   //// 2. LookAt Target = FrustumCenter in NDC space
   //FVector4 TargetWorld = FVector4(FrustumCenter, 1.0f);
   //FVector4 TargetNDC = CameraViewProj.TransformFVector4(TargetWorld);
   //TargetNDC = FVector4(TargetNDC.x / TargetNDC.w, TargetNDC.y / TargetNDC.w, TargetNDC.z / TargetNDC.w, 1.0f);
   
   // 2. LookAt Target = FrustumCenter in NDC space
   FVector4 TargetWorld = LightWorld + FVector4(LightDir.x, LightDir.y, LightDir.z, 0.0f);
   FVector4 TargetNDC = CameraViewProj.TransformFVector4(TargetWorld);
   TargetNDC = FVector4(TargetNDC.x / TargetNDC.w, TargetNDC.y / TargetNDC.w, TargetNDC.z / TargetNDC.w, 1.0f);

   // 3. NDC 공간 기준으로 LightDir 계산
   FVector LightDirInNDC = (TargetNDC.xyz() - LightNDC.xyz()).Normalize();
   FVector LightUpNDC = FVector(0, 1, 0); // NDC space에서도 위쪽 기준은 고정

   // 4. Light ViewMatrix 구성
   FMatrix LightViewInNDC = JungleMath::CreateViewMatrix(LightNDC.xyz(), TargetNDC.xyz(), LightUpNDC);

   FVector Min = +FLT_MAX, Max = -FLT_MAX;
   for (int i = 0; i < 8; ++i)
   {
       FVector p = LightViewInNDC.TransformPosition(WorldPoints[i]);
       Min.x = std::min(Min.x, p.x);
       Min.y = std::min(Min.y, p.y);
       Min.z = std::min(Min.z, p.z);

       Max.x = std::max(Max.x, p.x);
       Max.y = std::max(Max.y, p.y);
       Max.z = std::max(Max.z, p.z);
   }

   FVector BoxCenter = (Min + Max) * 0.5f;
   FVector BoxExtent = (Max - Min) * 0.5f;

   float nearZ = std::max(1.0f, Min.z); // LH 좌표계에서 z는 + 방향으로 멀어짐
   float farZ = Max.z;

   float height = BoxExtent.y * 2.0f;
   float fovY = 2.0f * atanf(height * 0.5f / nearZ);

   float width = BoxExtent.x * 2.0f;
   float aspect = width / height;

   const FMatrix ProjectionMatrix =  
       JungleMath::CreateProjectionMatrix(fovY, aspect, nearZ, farZ);  

   // Fix: Remove const qualifier to allow calling SetViewProjectionMatrix  
   const_cast<USpotLightComponent*>(LightComp)->SetViewMatrix(ViewMatrix);
   const_cast<USpotLightComponent*>(LightComp)->SetProjectionMatrix(ProjectionMatrix);

   return ViewMatrix * ProjectionMatrix * InvCameraViewProj;
}
