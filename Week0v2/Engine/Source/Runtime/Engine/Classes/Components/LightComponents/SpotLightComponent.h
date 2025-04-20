#pragma once
#include "PointLightComponent.h"

struct FSpotlightComponentInfo : public FPointLightComponentInfo
{
    DECLARE_ACTORCOMPONENT_INFO(FSpotlightComponentInfo);

    float InnerConeRad;
    float OuterConeRad;

    FSpotlightComponentInfo()
        : FPointLightComponentInfo()
        , InnerConeRad(0.0f)
        , OuterConeRad(0.768f) // radian
    {
        InfoType = TEXT("FSpotLightComponentInfo");
        ComponentType = TEXT("USpotLightComponent");
    }

    virtual void Copy(FActorComponentInfo& Other) override
    {
        FPointLightComponentInfo::Copy(Other);
        FSpotlightComponentInfo& SpotLightInfo = static_cast<FSpotlightComponentInfo&>(Other);
        SpotLightInfo.InnerConeRad = InnerConeRad;
        SpotLightInfo.OuterConeRad = OuterConeRad;
    }

    virtual void Serialize(FArchive& ar) const override
    {
        FPointLightComponentInfo::Serialize(ar);
        ar << InnerConeRad << OuterConeRad;
    }

    virtual void Deserialize(FArchive& ar) override
    {
        FPointLightComponentInfo::Deserialize(ar);
        ar >> InnerConeRad >> OuterConeRad;
    }
};

class USpotLightComponent : public UPointLightComponent
{
    DECLARE_CLASS(USpotLightComponent, UPointLightComponent)

public:
    USpotLightComponent();
    USpotLightComponent(const USpotLightComponent& Other);
    virtual ~USpotLightComponent() override;

protected:
    float InnerConeRad = 0.0f;
    float OuterConeRad = 0.768f;

public:
    float GetInnerConeRad() const { return InnerConeRad; }
    float GetOuterConeRad() const { return OuterConeRad; }

    void SetInnerConeAngle(float AngleDegrees);
    void SetOuterConeAngle(float AngleDegrees);

    virtual UObject* Duplicate() const override;
    virtual void DuplicateSubObjects(const UObject* Source) override;
    virtual void PostDuplicate() override;

    virtual std::shared_ptr<FActorComponentInfo> GetActorComponentInfo() override;
    virtual void LoadAndConstruct(const FActorComponentInfo& Info) override;

    void SetAtlasIndex(const uint32 InAtlasIndex) { AtlasIndex = InAtlasIndex; }
    uint32 GetAtlasIndex() const { return AtlasIndex; }
private:
    uint32 AtlasIndex = UINT_MAX;
};
