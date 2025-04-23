#pragma once
#include "Light.h"
class UPointLightComponent;

class APointLightActor : public ALight
{
    DECLARE_CLASS(APointLightActor, ALight)
public:
    APointLightActor();
    APointLightActor(const APointLightActor& Other);
    virtual ~APointLightActor() override = default;
    void BeginPlay() override;
    void Tick(float DeltaTime) override;
    void Destroyed() override;
    void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    bool Destroy() override;
    UObject* Duplicate() const override;
    void DuplicateSubObjects(const UObject* Source) override;
    void PostDuplicate() override;
    void LoadAndConstruct(const TArray<std::shared_ptr<FActorComponentInfo>>& InfoArray) override;
    FActorInfo GetActorInfo() override;
};

