#include "PointShadowMapRenderPass.h"

#include "EditorEngine.h"
#include "LightManager.h"
#include "Components/Mesh/StaticMesh.h"
#include "Components/PrimitiveComponents/MeshComponents/StaticMeshComponents/StaticMeshComponent.h"
#include "Math/JungleMath.h"
#include "Renderer/VBIBTopologyMapping.h"
#include "UnrealEd/EditorViewportClient.h"

FPointShadowMapRenderPass::FPointShadowMapRenderPass(const FName& InShaderName)
    : FShadowMapRenderPass(InShaderName)
{
    FRenderer& Renderer = GEngine->renderer;
    FRenderResourceManager* renderResourceManager = Renderer.GetResourceManager();
    const FGraphicsDevice& Graphics = GEngine->graphicDevice;

    ID3D11Buffer* SB = renderResourceManager->CreateStructuredBuffer<FLightVP>(MAX_POINT_LIGHTS * 6);
    ID3D11ShaderResourceView* SBSRV = renderResourceManager->CreateBufferSRV(SB, MAX_POINT_LIGHTS * 6);

    renderResourceManager->AddOrSetSRVStructuredBuffer(TEXT("PointLightVPMat"), SB);
    renderResourceManager->AddOrSetSRVStructuredBufferSRV(TEXT("PointLightVPMat"), SBSRV);

    CreateShadowMapResource();
}

FPointShadowMapRenderPass::~FPointShadowMapRenderPass()
{
}

void FPointShadowMapRenderPass::AddRenderObjectsToRenderPass(UWorld* InLevel)
{
    FShadowMapRenderPass::AddRenderObjectsToRenderPass(InLevel);
}

void FPointShadowMapRenderPass::UpdateLightStructuredBuffer(const std::shared_ptr<FViewportClient>& InViewportClient)
{
    FRenderer& Renderer = GEngine->renderer;
    FRenderResourceManager* renderResourceManager = Renderer.GetResourceManager();
    
    FLightManager* LightManager = Renderer.LightManager;
    TArray<UPointLightComponent*> VisiblePointLights = LightManager->GetVisiblePointLights();
    TArray<FLightVP> PointLightViewProjMatrices;

    std::shared_ptr<FEditorViewportClient> curEditorViewportClient = std::dynamic_pointer_cast<FEditorViewportClient>(InViewportClient);
    for (const UPointLightComponent* LightComp : VisiblePointLights)
    {
        if (!LightComp) continue;

        TArray<FMatrix> VPMats = GetLightViewProjectionMatrix(LightComp);
        for (int i = 0 ; i < VPMats.Num(); ++i)
        {
            FLightVP GPULight;
            GPULight.LightVP = VPMats[i];
            PointLightViewProjMatrices.Add(GPULight);
        }
    }

    renderResourceManager->UpdateStructuredBuffer(TEXT("PointLightVPMat"), PointLightViewProjMatrices);
}

void FPointShadowMapRenderPass::Prepare(std::shared_ptr<FViewportClient> InViewportClient)
{
    FShadowMapRenderPass::Prepare(InViewportClient);

    FRenderer& Renderer = GEngine->renderer;
    FRenderResourceManager* renderResourceManager = Renderer.GetResourceManager();
    const FGraphicsDevice& Graphics = GEngine->graphicDevice;
    
    D3D11_VIEWPORT shadowViewport = {};
    shadowViewport.TopLeftX = 0;
    shadowViewport.TopLeftY = 0;
    shadowViewport.Width = static_cast<float>(MapWidth);
    shadowViewport.Height = static_cast<float>(MapHeight);
    shadowViewport.MinDepth = 0.0f;
    shadowViewport.MaxDepth = 1.0f;

    Graphics.DeviceContext->RSSetViewports(1, &shadowViewport);

    ID3D11ShaderResourceView* SBSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("PointLightVPMat"));
    Graphics.DeviceContext->VSSetShaderResources(0, 1, &SBSRV);
    Graphics.DeviceContext->GSSetShaderResources(0, 1, &SBSRV);

    Graphics.DeviceContext->PSSetShader(nullptr, nullptr, 0);

    ID3D11DepthStencilState* DepthStencilState = Renderer.GetDepthStencilState(EDepthStencilState::LessEqual);
    Graphics.DeviceContext->OMSetDepthStencilState(DepthStencilState, 0);
    
    ID3D11DepthStencilView* ShadowMapDSVArray = renderResourceManager->GetShadowMapDSV(PointLightShadowMap);
    Graphics.DeviceContext->ClearDepthStencilView(ShadowMapDSVArray, D3D11_CLEAR_DEPTH, 1, 0);
    Graphics.DeviceContext->OMSetRenderTargets(0, nullptr, ShadowMapDSVArray);
}

void FPointShadowMapRenderPass::Execute(std::shared_ptr<FViewportClient> InViewportClient)
{
    FShadowMapRenderPass::Execute(InViewportClient);

    FRenderer& Renderer = GEngine->renderer;
    FGraphicsDevice& Graphics = GEngine->graphicDevice;

    FLightManager* LightManager = Renderer.LightManager;
    uint32 NumPointLights = LightManager->GetVisiblePointLights().Num();
    UpdateLightStructuredBuffer(InViewportClient);

    for (const UStaticMeshComponent* staticMeshComp : StaticMeshComponents)
    {
        const FMatrix Model = JungleMath::CreateModelMatrix(staticMeshComp->GetComponentLocation(), staticMeshComp->GetComponentRotation(),
            staticMeshComp->GetComponentScale());

        FPointCB pointCB;
        pointCB.ModelMatrix = Model;
        pointCB.NumPoints = NumPointLights;

        Renderer.GetResourceManager()->UpdateConstantBuffer(TEXT("FPointCB"), &pointCB);
        
        if (!staticMeshComp->GetStaticMesh()) continue;
        
        const OBJ::FStaticMeshRenderData* renderData = staticMeshComp->GetStaticMesh()->GetRenderData();
        if (!renderData) continue;

        const std::shared_ptr<FVBIBTopologyMapping> VBIBTopMappingInfo = Renderer.GetVBIBTopologyMapping(staticMeshComp->GetVBIBTopologyMappingName());
        VBIBTopMappingInfo->Bind();

        // If There's No Material Subset
        if (renderData->MaterialSubsets.Num() == 0)
        {
            Graphics.DeviceContext->DrawIndexedInstanced(VBIBTopMappingInfo->GetNumIndices(), NumPointLights, 0, 0, 0);
        }

        // SubSet마다 Material Update 및 Draw
        for (int subMeshIndex = 0; subMeshIndex < renderData->MaterialSubsets.Num(); ++subMeshIndex)
        {
            const int materialIndex = renderData->MaterialSubsets[subMeshIndex].MaterialIndex;

            // index draw
            const uint64 startIndex = renderData->MaterialSubsets[subMeshIndex].IndexStart;
            const uint64 indexCount = renderData->MaterialSubsets[subMeshIndex].IndexCount;
            Graphics.DeviceContext->DrawIndexedInstanced(indexCount, NumPointLights, startIndex, 0, 0);
        }
    }
}

void FPointShadowMapRenderPass::ClearRenderObjects()
{
    FShadowMapRenderPass::ClearRenderObjects();
}

TArray<FMatrix> FPointShadowMapRenderPass::GetLightViewProjectionMatrix(const UPointLightComponent* LightComp)
{
    TArray<FMatrix> ViewProjMatrices;
    const FVector Position = LightComp->GetOwner()->GetActorLocation();
    for (uint32_t face = 0; face < 6; ++face)
    {
        static const FVector FowardVector[6] = {
            {1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}
        };
        static const FVector UpVector[6] = {
            {0,1,0},{0,1,0},{0,0,-1},{0,0,1},{0,1,0},{0,1,0}
        };
        const FVector TargetPos = Position +  FowardVector[face];
        FMatrix view = JungleMath::CreateViewMatrix(Position, TargetPos, UpVector[face]);
        FMatrix proj = JungleMath::CreateProjectionMatrix(PIDIV2, 1.0f, LightComp->GetRadius() * 0.01f, LightComp->GetRadius());
        FMatrix vpMat = view * proj;
        ViewProjMatrices.Add(vpMat);
    }

    return ViewProjMatrices;
}

void FPointShadowMapRenderPass::CreateShadowMapResource() const
{
    const FRenderer& Renderer = GEngine->renderer;
    FRenderResourceManager* renderResourceManager = Renderer.GetResourceManager();
    
    ID3D11Texture2D* ShadowMapTextureCube2DArray = renderResourceManager->CreateTextureCube2DArray(MapWidth, MapHeight, MAX_POINT_LIGHTS);
    ID3D11DepthStencilView* ShadowMapDSVArray = renderResourceManager->CreateTextureCube2DArrayDSV(ShadowMapTextureCube2DArray, MAX_POINT_LIGHTS);
    ID3D11ShaderResourceView* ShadowMapSRVArray = renderResourceManager->CreateTextureCube2DArraySRV(ShadowMapTextureCube2DArray, MAX_POINT_LIGHTS) ;

    renderResourceManager->AddOrSetSRVShadowMapTexutre(PointLightShadowMap, ShadowMapTextureCube2DArray);
    renderResourceManager->AddOrSetSRVShadowMapSRV(PointLightShadowMap, ShadowMapSRVArray);
    renderResourceManager->AddOrSetDSVShadowMapTexutre(PointLightShadowMap, ShadowMapTextureCube2DArray);
    renderResourceManager->AddOrSetDSVShadowMapDSV(PointLightShadowMap, ShadowMapDSVArray);
}
