#pragma once
#include "Renderer/RenderPass/FBaseRenderPass.h"
#include "Define.h"

class UStaticMeshComponent;
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

protected:
    struct alignas(16) FCascadeCB
    {
        FMatrix ModelMatrix;
        FMatrix LightVP[MAX_CASCADES]; // per-cascade VP matrices
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
    
    struct FLightVP
    {
        FMatrix LightVP;
    };

    TArray<UStaticMeshComponent*> StaticMeshComponents;
};