#pragma once
#include "ShadowMapRenderPass.h"

class UDirectionalLightComponent;

class FDirectionalShadowMapRenderPass : public FShadowMapRenderPass
{
public:
    FDirectionalShadowMapRenderPass(const FName& InShaderName);
    ~FDirectionalShadowMapRenderPass() override;
    void AddRenderObjectsToRenderPass(UWorld* InLevel) override;
    void Execute(std::shared_ptr<FViewportClient> InViewportClient) override;
    void Prepare(std::shared_ptr<FViewportClient> InViewportClient) override;
    void ClearRenderObjects() override;
private:
    void CreateShadowMapResource();

    const uint32 MapWidth = 4096;
    const uint32 MapHeight = 4096;

    const FName DirLightShadowMap = TEXT("DirLightShadowMap");
};
