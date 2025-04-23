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
    const FMatrix& GetViewMatrix(int32 CascadeIndex) const
    {
        return ViewMatrix[CascadeIndex];
    }
    void SetViewMatrix(int32 CascadeIndex, const FMatrix& InViewMatrix)
    {
        ViewMatrix[CascadeIndex] = InViewMatrix;
    }
    const FMatrix& GetProjectionMatrix(int32 CascadeIndex) const
    {
        return ProjectionMatrix[CascadeIndex];
    }
    void SetProjectionMatrix(int32 CascadeIndex, const FMatrix& InProjectionMatrix)
    {
        ProjectionMatrix[CascadeIndex] = InProjectionMatrix;
    }
    FMatrix GetViewProjectionMatrix(int32 CascadeIndex) const
    {
        return ViewMatrix[CascadeIndex] * ProjectionMatrix[CascadeIndex];
    }
    const float* GetCascadeSplits() const
    {
        return CascadeSplits;
    }
    void SetCascadeSplits(const float* InCascadeSplits)
    {
        for (int32 i = 0; i < MAX_CASCADES + 1; ++i)
        {
            CascadeSplits[i] = InCascadeSplits[i];
        }
    }
public:
    virtual UObject* Duplicate() const override;
    virtual void DuplicateSubObjects(const UObject* Source) override;
    virtual void PostDuplicate() override;

public:
    virtual std::shared_ptr<FActorComponentInfo> GetActorComponentInfo() override;
    virtual void LoadAndConstruct(const FActorComponentInfo& Info) override;
private:
    FMatrix ViewMatrix[MAX_CASCADES];
    FMatrix ProjectionMatrix[MAX_CASCADES];
    float CascadeSplits[MAX_CASCADES + 1];
};


