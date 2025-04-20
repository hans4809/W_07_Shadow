#pragma once
#include "ShadowMapRenderPass.h"

class USpotLightComponent;

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
private:
    const uint32 MapWidth = 512;
    const uint32 MapHeight = 512;
    const FName ShadowMap = TEXT("SpotLightShadowMap");
};
