#pragma once
#include "Classes/GameFramework/Actor.h"

class UBillboardComponent;
class ULightComponentBase;

class ALight : public AActor
{
    DECLARE_CLASS(ALight, AActor)
public:
    ALight();
    ALight(const ALight& Other);
    virtual ~ALight() override;
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
    ULightComponentBase* GetLightComponent();
protected:
    ULightComponentBase* LightComponent;
    UBillboardComponent* BillboardComponent;
};

