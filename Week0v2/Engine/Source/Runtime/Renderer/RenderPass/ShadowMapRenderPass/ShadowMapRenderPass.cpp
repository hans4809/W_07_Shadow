#include "ShadowMapRenderPass.h"

#include "EditorEngine.h"
#include "BaseGizmos/GizmoBaseComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

FShadowMapRenderPass::FShadowMapRenderPass(const FName& InShaderName)
    : FBaseRenderPass(InShaderName)
{
}

FShadowMapRenderPass::~FShadowMapRenderPass()
{
}

void FShadowMapRenderPass::AddRenderObjectsToRenderPass(UWorld* InWorld)
{
    for (const AActor* actor : InWorld->GetActors())
    {
        for (const UActorComponent* actorComp : actor->GetComponents())
        {
            if (UStaticMeshComponent* pStaticMeshComp = Cast<UStaticMeshComponent>(actorComp))
            {
                if (!Cast<UGizmoBaseComponent>(actorComp))
                    StaticMeshComponents.Add(pStaticMeshComp);
            }
        }
    }
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
    StaticMeshComponents.Empty();
}
