#include "DirectionalShadowMapRenderPass.h"

#include "EditorEngine.h"

FDirectionalShadowMapRenderPass::FDirectionalShadowMapRenderPass(const FName& InShaderName)
    : FShadowMapRenderPass(InShaderName)
{
    FRenderer& Renderer = GEngine->renderer;
    FRenderResourceManager* renderResourceManager = Renderer.GetResourceManager();
    const FGraphicsDevice& Graphics = GEngine->graphicDevice;
    
    ID3D11Buffer* SB = nullptr;
    ID3D11ShaderResourceView* SBSRV = nullptr;
    SB = renderResourceManager->CreateStructuredBuffer<FMatrix>(MAX_CASCADES);
    SBSRV = renderResourceManager->CreateBufferSRV(SB, MAX_CASCADES);

    renderResourceManager->AddOrSetSRVStructuredBuffer(TEXT("DirectionalLightVPMat"), SB);
    renderResourceManager->AddOrSetSRVStructuredBufferSRV(TEXT("DirectionalLightVPMat"), SBSRV);
}

FDirectionalShadowMapRenderPass::~FDirectionalShadowMapRenderPass()
{
}

void FDirectionalShadowMapRenderPass::AddRenderObjectsToRenderPass(UWorld* InLevel)
{
    FShadowMapRenderPass::AddRenderObjectsToRenderPass(InLevel);
    // TODO : Directional LightComponent 추가
}

void FDirectionalShadowMapRenderPass::Prepare(std::shared_ptr<FViewportClient> InViewportClient)
{
    FShadowMapRenderPass::Prepare(InViewportClient);
}


void FDirectionalShadowMapRenderPass::Execute(std::shared_ptr<FViewportClient> InViewportClient)
{
    FShadowMapRenderPass::Execute(InViewportClient);
}

void FDirectionalShadowMapRenderPass::ClearRenderObjects()
{
    FShadowMapRenderPass::ClearRenderObjects();
}
