#pragma once

#include "Components/LightComponents/DirectionalLightComponent.h"
#include "Components/LightComponents/PointLightComponent.h"
#include "Components/LightComponents/SpotLightComponent.h"
#include "Engine/World.h"

class UAmbientLightComponent;
class UWorld;
class ULightComponentBase;

class FLightManager
{
public:
    void CollectLights(UWorld* InWorld);
    void CullLights(const FFrustum& ViewFrustum);
    void UploadLightConstants();
    void VisualizeLights();

    bool HasAmbientLight() const { return AmbientLight != nullptr; }
    bool HasDirectionalLight() const { return DirectionalLight != nullptr; }

    UAmbientLightComponent* GetAmbientLight() const { return AmbientLight; }
    ULightComponentBase* GetDirectionalLight() const { return DirectionalLight; }

    TArray<UPointLightComponent*>& GetVisiblePointLights() { return VisiblePointLights; }
    TArray<USpotLightComponent*>& GetVisibleSpotLights() { return VisibleSpotLights; }
private:
    UAmbientLightComponent* AmbientLight = nullptr;
    UDirectionalLightComponent* DirectionalLight = nullptr;
    
    TArray<UPointLightComponent*> AllPointLights;
    TArray<USpotLightComponent*> AllSpotLights;

    TArray<UPointLightComponent*> VisiblePointLights;
    TArray<USpotLightComponent*> VisibleSpotLights;

    bool IsSpotLightInFrustum(USpotLightComponent* SpotLightComp, const FFrustum& CameraFrustum) const;
};
