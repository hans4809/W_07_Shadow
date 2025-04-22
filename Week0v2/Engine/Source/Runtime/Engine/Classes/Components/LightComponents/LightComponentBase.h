#pragma once
#include "Define.h"
#include "Components/SceneComponent.h"
#include "UObject/ObjectMacros.h"

class ID3D11ShaderResourceView;
struct FLightComponentBaseInfo : public FSceneComponentInfo
{
    DECLARE_ACTORCOMPONENT_INFO(FLightComponentBaseInfo);

    FVector4 Color;
    float Intensity;
    bool bCastShadows;

    // ctor
    FLightComponentBaseInfo()
        : FSceneComponentInfo()
        , Color(FVector4(1, 1, 1, 1))
        , Intensity(1.0f)
        , bCastShadows(false)
    {
        InfoType = TEXT("FLightComponentBaseInfo");
        ComponentType = TEXT("ULightComponentBase");
    }

    virtual void Copy(FActorComponentInfo& Other) override
    {
        FSceneComponentInfo::Copy(Other);
        FLightComponentBaseInfo& LightInfo = static_cast<FLightComponentBaseInfo&>(Other);
        LightInfo.Color = Color;
        LightInfo.Intensity = Intensity;
        LightInfo.bCastShadows = bCastShadows;
    }

    virtual void Serialize(FArchive& ar) const override
    {
        FSceneComponentInfo::Serialize(ar);
        ar << Color << Intensity << bCastShadows;
    }

    virtual void Deserialize(FArchive& ar) override
    {
        FSceneComponentInfo::Deserialize(ar);
        ar >> Color >> Intensity >> bCastShadows;
    }
};

class ULightComponentBase : public USceneComponent
{
    DECLARE_CLASS(ULightComponentBase, USceneComponent)

public:
    ULightComponentBase();
    ULightComponentBase(const ULightComponentBase& Other);
    virtual ~ULightComponentBase() override;

    virtual void TickComponent(float DeltaTime) override;
    virtual int CheckRayIntersection(FVector& rayOrigin, FVector& rayDirection, float& pfNearHitDistance);
    virtual void InitializeComponent() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    void InitializeLight();
    void SetColor(FVector4 newColor);
    FVector4 GetColor() const;

    TArray<ID3D11ShaderResourceView*> ShadowSRVSlice;
protected:
    FVector4 LightColor = { 1, 1, 1, 1 }; // RGBA
    float Intensity = 1.0f;
    bool bCastShadows = false;

public:
    FVector4 GetLightColor() const { return LightColor; }
    float GetIntensity() const { return Intensity; }
    void SetIntensity(float InIntensity) { Intensity = InIntensity; }
    bool CanCastShadows() const { return bCastShadows; }
    void SetCastShadows(const bool InbCastShadows) { bCastShadows = InbCastShadows; }
public:
    // duplictae
    virtual UObject* Duplicate() const override;
    virtual void DuplicateSubObjects(const UObject* Source) override;
    virtual void PostDuplicate() override;

public:
    virtual std::shared_ptr<FActorComponentInfo> GetActorComponentInfo() override;
    virtual void LoadAndConstruct(const FActorComponentInfo& Info) override;
};

