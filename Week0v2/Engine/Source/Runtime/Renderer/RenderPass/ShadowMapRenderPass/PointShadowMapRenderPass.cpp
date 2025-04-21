#include "PointShadowMapRenderPass.h"

#include "EditorEngine.h"

FPointShadowMapRenderPass::FPointShadowMapRenderPass(const FName& InShaderName)
    : FShadowMapRenderPass(InShaderName)
{
    FRenderer& Renderer = GEngine->renderer;
    FRenderResourceManager* renderResourceManager = Renderer.GetResourceManager();
    const FGraphicsDevice& Graphics = GEngine->graphicDevice;
    
    ID3D11Buffer* SB = nullptr;
    ID3D11ShaderResourceView* SBSRV = nullptr;
    SB = renderResourceManager->CreateStructuredBuffer<FMatrix>(MAX_POINT_LIGHTS);
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
    // TODO : PointLightComp 추가
}

void FPointShadowMapRenderPass::Prepare(std::shared_ptr<FViewportClient> InViewportClient)
{
    FShadowMapRenderPass::Prepare(InViewportClient);

    FRenderer& Renderer = GEngine->renderer;
    FRenderResourceManager* renderResourceManager = Renderer.GetResourceManager();
    const FGraphicsDevice& Graphics = GEngine->graphicDevice;
    
    ID3D11ShaderResourceView* SBSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("PointLightVPMat"));
    Graphics.DeviceContext->VSSetShaderResources(0, 1, &SBSRV);

    // TODO : GS SET
}

void FPointShadowMapRenderPass::Execute(std::shared_ptr<FViewportClient> InViewportClient)
{
    FShadowMapRenderPass::Execute(InViewportClient);
}

void FPointShadowMapRenderPass::ClearRenderObjects()
{
    FShadowMapRenderPass::ClearRenderObjects();
}
