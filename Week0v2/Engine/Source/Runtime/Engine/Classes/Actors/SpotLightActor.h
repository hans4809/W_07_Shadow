#pragma once
#include "Light.h"

class ASpotLightActor : public ALight
{
    DECLARE_CLASS(ASpotLightActor, ALight)
public:
    ASpotLightActor();
    ASpotLightActor(const ASpotLightActor& Other);
    virtual ~ASpotLightActor() override = default;
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