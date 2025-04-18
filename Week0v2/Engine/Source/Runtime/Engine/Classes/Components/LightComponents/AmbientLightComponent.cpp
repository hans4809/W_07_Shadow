#include "AmbientLightComponent.h"
#include "UObject/ObjectFactory.h"
#include "CoreUObject/UObject/Casts.h"

UAmbientLightComponent::UAmbientLightComponent()
{
}

UAmbientLightComponent::UAmbientLightComponent(const UAmbientLightComponent& Other)
    : Super(Other)
{
}

//void UAmbientLightComponent::SetDirection(FVector _newDir)
//{
//    //잘 안됨
//    FVector Axis = Direction.Cross(_newDir).Normalize();
//    float Angle = acosf(Direction.Normalize().Dot(_newDir.Normalize()));
//    GetOwner()->GetRootComponent()->SetRelativeQuat(FQuat::FromAxisAngle(Axis, Angle));
//    Direction = _newDir;
//}

UObject* UAmbientLightComponent::Duplicate() const
{
    UAmbientLightComponent* NewComp = FObjectFactory::ConstructObjectFrom<UAmbientLightComponent>(this);
    NewComp->DuplicateSubObjects(this);
    NewComp->PostDuplicate();

    return NewComp;
}

void UAmbientLightComponent::DuplicateSubObjects(const UObject* Source)
{
    Super::DuplicateSubObjects(Source);
    // 여기서 복사할 것? 방향뿐임
    UAmbientLightComponent* SourceComp = Cast<UAmbientLightComponent>(Source);
}

void UAmbientLightComponent::PostDuplicate()
{
}

std::shared_ptr<FActorComponentInfo> UAmbientLightComponent::GetActorComponentInfo()
{
    std::shared_ptr<FAmbientLightComponentInfo> Info = std::make_shared<FAmbientLightComponentInfo>();
    Super::GetActorComponentInfo()->Copy(*Info);
    return Info;
}

void UAmbientLightComponent::LoadAndConstruct(const FActorComponentInfo& Info)
{
    Super::LoadAndConstruct(Info);
    const FAmbientLightComponentInfo& AmbientLightInfo = static_cast<const FAmbientLightComponentInfo&>(Info);
}
