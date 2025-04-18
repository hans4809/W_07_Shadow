#include "SpotLightComponent.h"
#include "UObject/ObjectFactory.h"
#include "Math/JungleMath.h"

USpotLightComponent::USpotLightComponent()
    : Super()
{
}

USpotLightComponent::USpotLightComponent(const USpotLightComponent& Other)
    : Super(Other)
    , InnerConeAngle(Other.InnerConeAngle)
    , OuterConeAngle(Other.OuterConeAngle)
{
}

void USpotLightComponent::SetInnerConeAngle(float AngleDegrees)
{
    InnerConeAngle = JungleMath::DegToRad(AngleDegrees);
}

void USpotLightComponent::SetOuterConeAngle(float AngleDegrees)
{
    OuterConeAngle = JungleMath::DegToRad(AngleDegrees);
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

    Info->InnerConeAngle = InnerConeAngle;
    Info->OuterConeAngle = OuterConeAngle;

    return Info;
}

void USpotLightComponent::LoadAndConstruct(const FActorComponentInfo& Info)
{
    Super::LoadAndConstruct(Info);
    const FSpotlightComponentInfo& SpotInfo = static_cast<const FSpotlightComponentInfo&>(Info);

    InnerConeAngle = SpotInfo.InnerConeAngle;
    OuterConeAngle = SpotInfo.OuterConeAngle;
}
