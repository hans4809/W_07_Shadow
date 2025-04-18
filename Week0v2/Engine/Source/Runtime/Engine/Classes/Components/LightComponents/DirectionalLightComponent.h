#pragma once
#include "LightComponent.h"
#include "Actors/Light.h"

struct FDirectionalLightComponentInfo : public FLightComponentInfo
{
    DECLARE_ACTORCOMPONENT_INFO(FDirectionalLightComponentInfo);


    FDirectionalLightComponentInfo()
        : FLightComponentInfo()
    {
        InfoType = TEXT("FDirectionalLightComponentInfo");
        ComponentType = TEXT("UDirectionalLightComponent");
    }

    virtual void Copy(FActorComponentInfo& Other) override
    {
        FLightComponentInfo::Copy(Other);
        FDirectionalLightComponentInfo& DirectionalLightInfo = static_cast<FDirectionalLightComponentInfo&>(Other);
    }
    virtual void Serialize(FArchive& ar) const override
    {
        FLightComponentInfo::Serialize(ar);
    }
    virtual void Deserialize(FArchive& ar) override
    {
        FLightComponentInfo::Deserialize(ar);
    }
};
class UDirectionalLightComponent : public ULightComponent
{
    DECLARE_CLASS(UDirectionalLightComponent, ULightComponent)
public:
    UDirectionalLightComponent();
    UDirectionalLightComponent(const UDirectionalLightComponent& Other);
    virtual ~UDirectionalLightComponent() override = default;
public:

public:
    virtual UObject* Duplicate() const override;
    virtual void DuplicateSubObjects(const UObject* Source) override;
    virtual void PostDuplicate() override;

public:
    virtual std::shared_ptr<FActorComponentInfo> GetActorComponentInfo() override;
    virtual void LoadAndConstruct(const FActorComponentInfo& Info) override;
};


