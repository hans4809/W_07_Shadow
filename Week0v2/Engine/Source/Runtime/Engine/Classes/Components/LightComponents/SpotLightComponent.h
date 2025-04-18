#pragma once
#include "DirectionalLightComponent.h"

struct FSpotlightComponentInfo : public FDirectionalLightComponentInfo
{
    DECLARE_ACTORCOMPONENT_INFO(FSpotlightComponentInfo);

    float InnerConeAngle;
    float OuterConeAngle;

    float Radius;
    float AttenuationFalloff;
    FSpotlightComponentInfo()
        :FDirectionalLightComponentInfo()
        , InnerConeAngle(0.0f)
        , OuterConeAngle(0.768f)
        , Radius(1.0f)
        , AttenuationFalloff(0.01f)
    {
        InfoType = TEXT("FSpotLightComponentInfo");
        ComponentType = TEXT("USpotLightComponent");
    }

    virtual void Copy(FActorComponentInfo& Other) override
    {
        FDirectionalLightComponentInfo::Copy(Other);
        FSpotlightComponentInfo& SpotLightInfo = static_cast<FSpotlightComponentInfo&>(Other);
        SpotLightInfo.InnerConeAngle = InnerConeAngle;
        SpotLightInfo.OuterConeAngle = OuterConeAngle;
        SpotLightInfo.Radius = Radius;
        SpotLightInfo.AttenuationFalloff = AttenuationFalloff;
    }

    virtual void Serialize(FArchive& ar) const override
    {
        FDirectionalLightComponentInfo::Serialize(ar);
        ar << InnerConeAngle << OuterConeAngle<<Radius<<AttenuationFalloff;
    }

    virtual void Deserialize(FArchive& ar) override
    {
        FDirectionalLightComponentInfo::Deserialize(ar);
        ar >> InnerConeAngle >> OuterConeAngle>>Radius>>AttenuationFalloff;
    }
};

class USpotLightComponent : public UDirectionalLightComponent
{
    DECLARE_CLASS(USpotLightComponent, UDirectionalLightComponent)
public:
    USpotLightComponent();
    USpotLightComponent(const USpotLightComponent& Other);
    virtual ~USpotLightComponent() override = default;
protected:
    //angle은 내부적으로 radian

    float Radius = 1.0f;
    float AttenuationFalloff = 0.01f;

    float InnerConeAngle = 0.0f;
    float OuterConeAngle = 0.768f;

public:
    float GetInnerConeAngle() const { return InnerConeAngle; }
    float GetOuterConeAngle() const { return OuterConeAngle; }
    //외부에서 set 해줄때는 degree로 들어옴
    void SetInnerConeAngle(float Angle);
    void SetOuterConeAngle(float Angle);

    float GetRadius() const { return Radius; }
    void SetRadius(const float InRadius) { Radius = InRadius; }
    float GetAttenuation() const { return 1.0f / AttenuationFalloff * (Radius * Radius); }
    float GetAttenuationFalloff() const { return AttenuationFalloff; }
    void SetAttenuationFallOff(const float InAttenuationFalloff) { AttenuationFalloff = InAttenuationFalloff; }


    virtual UObject* Duplicate() const override;
    virtual void DuplicateSubObjects(const UObject* Source) override;
    virtual void PostDuplicate() override;

public:
    virtual std::shared_ptr<FActorComponentInfo> GetActorComponentInfo() override;
    virtual void LoadAndConstruct(const FActorComponentInfo& Info) override;
};