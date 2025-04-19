#include "DirectionalShadowMapRenderPass.h"

#include "EditorEngine.h"

FDirectionalShadowMapRenderPass::FDirectionalShadowMapRenderPass(const FName& InShaderName)
    : FShadowMapRenderPass(InShaderName)
{
}

FDirectionalShadowMapRenderPass::~FDirectionalShadowMapRenderPass()
{
}

void FDirectionalShadowMapRenderPass::AddRenderObjectsToRenderPass(UWorld* InLevel)
{
    FShadowMapRenderPass::AddRenderObjectsToRenderPass(InLevel);
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
