#pragma once
#include "FBaseRenderPass.h"
#include "Container/Array.h"

struct FMatrix;
class UStaticMeshComponent;

class FDepthPrePass : FBaseRenderPass
{
public:
    FDepthPrePass(const FName& InShaderName);
    ~FDepthPrePass() override;
    void AddRenderObjectsToRenderPass(UWorld* InWorld) override;
    void Prepare(std::shared_ptr<FViewportClient> InViewportClient) override;
    void Execute(std::shared_ptr<FViewportClient> InViewportClient) override;
    void ClearRenderObjects() override;
private:
    static void UpdateMatrixConstants(UStaticMeshComponent* InStaticMeshComponent, const FMatrix& InView, const FMatrix& InProjection);

    TArray<UStaticMeshComponent*> StaticMesheComponents;
};
