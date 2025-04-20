#include "SpotShadowMapRenderPass.h"

#include "EditorEngine.h"

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

    ID3D11ShaderResourceView* SBSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("SpotLightVPMat"));
    Graphics.DeviceContext->VSSetShaderResources(0, 1, &SBSRV);

    ID3D11DepthStencilState* DepthStencilState =
        Renderer.GetDepthStencilState(EDepthStencilState::LessEqual);
    Graphics.DeviceContext->PSSetShader(nullptr, nullptr, 0);
    Graphics.DeviceContext->ClearDepthStencilView(Graphics.DirShadowDSV, D3D11_CLEAR_DEPTH, 1, 0);
    Graphics.DeviceContext->OMSetDepthStencilState(DepthStencilState, 0);
    Graphics.DeviceContext->OMSetRenderTargets(0, nullptr, Graphics.DirShadowDSV);
}

void FSpotShadowMapRenderPass::Execute(std::shared_ptr<FViewportClient> InViewportClient)
{
    FShadowMapRenderPass::Execute(InViewportClient);
}

void FSpotShadowMapRenderPass::ClearRenderObjects()
{
    FShadowMapRenderPass::ClearRenderObjects();
}
