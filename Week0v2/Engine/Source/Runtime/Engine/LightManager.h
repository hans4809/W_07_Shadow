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
    void CollectLights(const UWorld* InWorld);
    void CullLights(const FFrustum& ViewFrustum);
    void UploadLightConstants();
    void VisualizeLights();

    bool HasAmbientLight() const { return AmbientLight != nullptr; }
    bool HasDirectionalLight() const { return DirectionalLight != nullptr; }

    UAmbientLightComponent* GetAmbientLight() const { return AmbientLight; }
    UDirectionalLightComponent* GetDirectionalLight() const { return DirectionalLight; }

    TArray<UPointLightComponent*>& GetVisiblePointLights() { return VisiblePointLights; }
    TArray<USpotLightComponent*>& GetVisibleSpotLights() { return VisibleSpotLights; }

    uint32 GetAmbientLightNum() const;
    uint32 GetDirectionalLightNum() const;

    uint32 GetPointLightNum() const;
    uint32 GetSpotLightNum() const;
private:
    UAmbientLightComponent* AmbientLight = nullptr;
    UDirectionalLightComponent* DirectionalLight = nullptr;
    
    TArray<UPointLightComponent*> AllPointLights;
    TArray<USpotLightComponent*> AllSpotLights;

    TArray<UPointLightComponent*> VisiblePointLights;
    TArray<USpotLightComponent*> VisibleSpotLights;

    static bool IsSpotLightInFrustum(const USpotLightComponent* SpotLightComp, const FFrustum& CameraFrustum);
};
