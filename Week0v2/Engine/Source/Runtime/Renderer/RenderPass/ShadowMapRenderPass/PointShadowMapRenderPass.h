#pragma once
#include "ShadowMapRenderPass.h"

class UPointLightComponent;

class FPointShadowMapRenderPass : public FShadowMapRenderPass
{
public:
    FPointShadowMapRenderPass(const FName& InShaderName);
    ~FPointShadowMapRenderPass() override;
    void AddRenderObjectsToRenderPass(UWorld* InLevel) override;
    void UpdateLightStructuredBuffer(const std::shared_ptr<FViewportClient>& InViewportClient);
    void Execute(std::shared_ptr<FViewportClient> InViewportClient) override;
    void Prepare(std::shared_ptr<FViewportClient> InViewportClient) override;
    void ClearRenderObjects() override;
private:
    void CreateShadowMapResource() const;
    static TArray<FMatrix> GetLightViewProjectionMatrix(const UPointLightComponent* LightComp);
    
    const uint32 MapWidth = 512;
    const uint32 MapHeight = 512;
    const FName ShadowMap = TEXT("PointLightShadowMap");
};
