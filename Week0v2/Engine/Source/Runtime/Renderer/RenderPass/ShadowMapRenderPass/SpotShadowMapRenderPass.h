#pragma once
#include "ShadowMapRenderPass.h"

class USpotLightComponent;
class FLightManager;
class FSpotShadowMapRenderPass : public FShadowMapRenderPass
{
public:
    FSpotShadowMapRenderPass(const FName& InShaderName);
    ~FSpotShadowMapRenderPass() override;
    void AddRenderObjectsToRenderPass(UWorld* InLevel) override;
    void Execute(std::shared_ptr<FViewportClient> InViewportClient) override;
    void Prepare(std::shared_ptr<FViewportClient> InViewportClient) override;
    void ClearRenderObjects() override;
private:
    void CreateShadowMapResource();
    void UpdateLightStructuredBuffer(std::shared_ptr<FViewportClient> InViewportClient);
    FMatrix ComputeViewProj(const USpotLightComponent* LightComp);
    FMatrix ComputeViewProjPSM(const USpotLightComponent* LightComp,
        std::shared_ptr<FViewportClient> InViewportClient);
private:
    const uint32 MapWidth = 1024;
    const uint32 MapHeight = 1024;
    const FName SpotLightShadowMap = TEXT("SpotLightShadowMap");
    const FName SpotLightVPMat = TEXT("SpotLightVPMat");

    ID3D11RasterizerState* rasterizerState = nullptr;

};
