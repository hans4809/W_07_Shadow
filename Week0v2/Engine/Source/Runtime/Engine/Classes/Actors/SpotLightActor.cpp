#include "SpotLightActor.h"

#include "Components/LightComponents/SpotLightComponent.h"
#include "Components/PrimitiveComponents/UBillboardComponent.h"

ASpotLightActor::ASpotLightActor()
{
    LightComponent = AddComponent<USpotLightComponent>();
    RootComponent->SetRelativeRotation(FVector(0, 89.0f, 0));
    BillboardComponent->SetTexture(L"Assets/Texture/SpotLight_64x.png");
}

ASpotLightActor::ASpotLightActor(const ASpotLightActor& Other)
    : Super(Other)
{
}
void ASpotLightActor::BeginPlay()
{
    Super::BeginPlay();
}
void ASpotLightActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}
void ASpotLightActor::Destroyed()
{
    Super::Destroyed();
}
void ASpotLightActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}
bool ASpotLightActor::Destroy()
{
    return Super::Destroy();
}

UObject* ASpotLightActor::Duplicate() const
{
    ALight* NewActor = FObjectFactory::ConstructObjectFrom<ALight>(this);
    NewActor->DuplicateSubObjects(this);
    NewActor->PostDuplicate();
    return NewActor;
}
void ASpotLightActor::DuplicateSubObjects(const UObject* Source)
{
    Super::DuplicateSubObjects(Source);
}
void ASpotLightActor::PostDuplicate()
{
    Super::PostDuplicate();
}
void ASpotLightActor::LoadAndConstruct(const TArray<std::shared_ptr<FActorComponentInfo>>& InfoArray)
{
    Super::LoadAndConstruct(InfoArray);
}
FActorInfo ASpotLightActor::GetActorInfo()
{
    return Super::GetActorInfo();
}