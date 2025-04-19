#include "AmbientLightActor.h"

#include "Components/LightComponents/AmbientLightComponent.h"
#include "Components/PrimitiveComponents/UBillboardComponent.h"


AAmbientLightActor::AAmbientLightActor()
    : Super()
{
    LightComponent = AddComponent<UAmbientLightComponent>();
    BillboardComponent->SetTexture(L"Assets/Texture/S_LightRect.PNG");
}

AAmbientLightActor::AAmbientLightActor(const AAmbientLightActor& Other)
    : Super(Other)
{

}

void AAmbientLightActor::BeginPlay()
{
    Super::BeginPlay();
}

void AAmbientLightActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AAmbientLightActor::Destroyed()
{
    Super::Destroyed();
}

void AAmbientLightActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

bool AAmbientLightActor::Destroy()
{
    return Super::Destroy();
}

UObject* AAmbientLightActor::Duplicate() const
{
    ALight* NewActor = FObjectFactory::ConstructObjectFrom<ALight>(this);
    NewActor->DuplicateSubObjects(this);
    NewActor->PostDuplicate();
    return NewActor;
}

void AAmbientLightActor::DuplicateSubObjects(const UObject* Source)
{
    Super::DuplicateSubObjects(Source);
}

void AAmbientLightActor::PostDuplicate()
{
    Super::PostDuplicate();
}

void AAmbientLightActor::LoadAndConstruct(const TArray<std::shared_ptr<FActorComponentInfo>>& InfoArray)
{
    Super::LoadAndConstruct(InfoArray);
}

FActorInfo AAmbientLightActor::GetActorInfo()
{
    return Super::GetActorInfo();
}
