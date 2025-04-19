#pragma once
#include "ShadowMapRenderPass.h"

class UPointLightComponent;

class FPointShadowMapRenderPass : public FShadowMapRenderPass
{
public:
    FPointShadowMapRenderPass(const FName& InShaderName);
    ~FPointShadowMapRenderPass() override;
    void AddRenderObjectsToRenderPass(UWorld* InLevel) override;
    void Execute(std::shared_ptr<FViewportClient> InViewportClient) override;
    void Prepare(std::shared_ptr<FViewportClient> InViewportClient) override;
    void ClearRenderObjects() override;
private:
    TArray<UPointLightComponent*> PointLightComponents;
};
