#pragma once
#include "Container/Array.h"
#include "Math/Matrix.h"
#include "Renderer/RenderPass/FBaseRenderPass.h"

// 임시
#define MAX_CASCADES 4
#define MAX_SPOT_LIGHTS 16
#define MAX_POINT_LIGHTS 16

class ULightComponent;

class FShadowMapRenderPass : public FBaseRenderPass
{
public:
    FShadowMapRenderPass(const FName& InShaderName);
    ~FShadowMapRenderPass() override;
    
    void AddRenderObjectsToRenderPass(UWorld* InLevel) override;
    void Execute(std::shared_ptr<FViewportClient> InViewportClient) override;

    void Prepare(std::shared_ptr<FViewportClient> InViewportClient) override;
    void ClearRenderObjects() override;

private:
    struct alignas(16) FCascadeCB
    {
        FMatrix ModelMatrix;
        FMatrix LightViewProjectionMatrix[MAX_CASCADES]; // per-cascade VP matrices
        uint32  NumCascades;
        FVector pad;
    };

     struct alignas(16) FSpotCB
    {
        FMatrix ModelMatrix;
        uint32  NumSpotLights;
        FVector pad;
    };

    struct alignas(16) FPointCB
    {
        FMatrix ModelMatrix;
        uint32  NumPoints;
    };
};