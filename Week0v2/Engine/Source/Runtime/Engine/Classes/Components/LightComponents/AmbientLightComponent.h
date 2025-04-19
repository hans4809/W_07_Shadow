#pragma once
#include "LightComponent.h"
#include "Actors/Light.h"

struct FAmbientLightComponentInfo : public FLightComponentInfo
{
    DECLARE_ACTORCOMPONENT_INFO(FAmbientLightComponentInfo);


    FAmbientLightComponentInfo()
        : FLightComponentInfo()
    {
        InfoType = TEXT("FAmbientLightComponentInfo");
        ComponentType = TEXT("UAmbientLightComponent");
    }

    virtual void Copy(FActorComponentInfo& Other) override
    {
        FLightComponentInfo::Copy(Other);
        FAmbientLightComponentInfo& AmbientLightInfo = static_cast<FAmbientLightComponentInfo&>(Other);
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
class UAmbientLightComponent : public ULightComponent
{
    DECLARE_CLASS(UAmbientLightComponent, ULightComponent)
public:
    UAmbientLightComponent();
    UAmbientLightComponent(const UAmbientLightComponent& Other);
    virtual ~UAmbientLightComponent() override = default;
private:
public:

public:
    virtual UObject* Duplicate() const override;
    virtual void DuplicateSubObjects(const UObject* Source) override;
    virtual void PostDuplicate() override;

public:
    virtual std::shared_ptr<FActorComponentInfo> GetActorComponentInfo() override;
    virtual void LoadAndConstruct(const FActorComponentInfo& Info) override;
};


