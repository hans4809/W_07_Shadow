#include "SpotLightComponent.h"
#include "UObject/ObjectFactory.h"
#include "Math/JungleMath.h"

USpotLightComponent::USpotLightComponent()
    : Super()
{
}

USpotLightComponent::USpotLightComponent(const USpotLightComponent& Other)
    : Super(Other)
    , InnerConeRad(Other.InnerConeRad)
    , OuterConeRad(Other.OuterConeRad)
{
}

USpotLightComponent::~USpotLightComponent()
{
}

void USpotLightComponent::SetInnerConeAngle(float AngleDegrees)
{
    InnerConeRad = JungleMath::DegToRad(AngleDegrees);
}

void USpotLightComponent::SetOuterConeAngle(float AngleDegrees)
{
    OuterConeRad = JungleMath::DegToRad(AngleDegrees);
}

UObject* USpotLightComponent::Duplicate() const
{
    USpotLightComponent* NewComp = FObjectFactory::ConstructObjectFrom<USpotLightComponent>(this);
    NewComp->DuplicateSubObjects(this);
    NewComp->PostDuplicate();
    return NewComp;
}

void USpotLightComponent::DuplicateSubObjects(const UObject* Source)
{
    Super::DuplicateSubObjects(Source);
}

void USpotLightComponent::PostDuplicate()
{
    // Custom duplication logic if needed
}

std::shared_ptr<FActorComponentInfo> USpotLightComponent::GetActorComponentInfo()
{
    std::shared_ptr<FSpotlightComponentInfo> Info = std::make_shared<FSpotlightComponentInfo>();
    Super::GetActorComponentInfo()->Copy(*Info);

    Info->InnerConeRad = InnerConeRad;
    Info->OuterConeRad = OuterConeRad;

    return Info;
}

void USpotLightComponent::LoadAndConstruct(const FActorComponentInfo& Info)
{
    Super::LoadAndConstruct(Info);
    const FSpotlightComponentInfo& SpotInfo = static_cast<const FSpotlightComponentInfo&>(Info);

    InnerConeRad = SpotInfo.InnerConeRad;
    OuterConeRad = SpotInfo.OuterConeRad;
}
