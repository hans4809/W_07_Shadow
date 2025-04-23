#pragma once
#include "PointLightComponent.h"

struct FSpotlightComponentInfo : public FPointLightComponentInfo
{
    DECLARE_ACTORCOMPONENT_INFO(FSpotlightComponentInfo);

    float InnerConeAngle;
    float OuterConeAngle;

    FSpotlightComponentInfo()
        : FPointLightComponentInfo()
        , InnerConeAngle(0.0f)
        , OuterConeAngle(0.768f) // radian
    {
        InfoType = TEXT("FSpotLightComponentInfo");
        ComponentType = TEXT("USpotLightComponent");
    }

    virtual void Copy(FActorComponentInfo& Other) override
    {
        FPointLightComponentInfo::Copy(Other);
        FSpotlightComponentInfo& SpotLightInfo = static_cast<FSpotlightComponentInfo&>(Other);
        SpotLightInfo.InnerConeAngle = InnerConeAngle;
        SpotLightInfo.OuterConeAngle = OuterConeAngle;
    }

    virtual void Serialize(FArchive& ar) const override
    {
        FPointLightComponentInfo::Serialize(ar);
        ar << InnerConeAngle << OuterConeAngle;
    }

    virtual void Deserialize(FArchive& ar) override
    {
        FPointLightComponentInfo::Deserialize(ar);
        ar >> InnerConeAngle >> OuterConeAngle;
    }
};

class USpotLightComponent : public UPointLightComponent
{
    DECLARE_CLASS(USpotLightComponent, UPointLightComponent)

public:
    USpotLightComponent();
    USpotLightComponent(const USpotLightComponent& Other);
    virtual ~USpotLightComponent() override = default;

protected:
    float InnerConeAngle = 0.0f;
    float OuterConeAngle = 0.768f;

public:
    float GetInnerConeAngle() const { return InnerConeAngle; }
    float GetOuterConeAngle() const { return OuterConeAngle; }

    void SetInnerConeAngle(float AngleDegrees);
    void SetOuterConeAngle(float AngleDegrees);

    virtual UObject* Duplicate() const override;
    virtual void DuplicateSubObjects(const UObject* Source) override;
    virtual void PostDuplicate() override;

    virtual std::shared_ptr<FActorComponentInfo> GetActorComponentInfo() override;
    virtual void LoadAndConstruct(const FActorComponentInfo& Info) override;

public:
    const FMatrix& GetViewMatrix() const
    {
        return ViewMatrix;
    }
    void SetViewMatrix(const FMatrix& InViewMatrix)
    {
        ViewMatrix = InViewMatrix;
    }

    const FMatrix& GetProjectionMatrix() const
    {
        return ProjectionMatrix;
    }
    void SetProjectionMatrix(const FMatrix& InProjectionMatrix)
    {
        ProjectionMatrix = InProjectionMatrix;
    }

private:
    FMatrix ViewMatrix;
    FMatrix ProjectionMatrix;
};
