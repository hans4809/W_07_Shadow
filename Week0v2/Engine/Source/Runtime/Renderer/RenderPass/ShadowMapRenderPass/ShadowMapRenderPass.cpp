#include "ShadowMapRenderPass.h"

#include "EditorEngine.h"

FShadowMapRenderPass::FShadowMapRenderPass(const FName& InShaderName)
    : FBaseRenderPass(InShaderName)
{
}

FShadowMapRenderPass::~FShadowMapRenderPass()
{
}

void FShadowMapRenderPass::AddRenderObjectsToRenderPass(UWorld* InLevel)
{
    // TODO : LightComp 추가
}

void FShadowMapRenderPass::Prepare(std::shared_ptr<FViewportClient> InViewportClient)
{
    FBaseRenderPass::Prepare(InViewportClient);
    // 공통적인 ShadowMap Prepare 처리
}

void FShadowMapRenderPass::Execute(std::shared_ptr<FViewportClient> InViewportClient)
{
    
}

void FShadowMapRenderPass::ClearRenderObjects()
{
    FBaseRenderPass::ClearRenderObjects();
}
