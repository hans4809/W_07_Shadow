#include "LightManager.h"

#include "EditorEngine.h"
#include "Components/LightComponents/AmbientLightComponent.h"
#include "D3D11RHI/CBStructDefine.h"
#include "Renderer/RenderResourceManager.h"

void FLightManager::CollectLights(UWorld* InWorld)
{
    AllPointLights.Empty();
    AllSpotLights.Empty();
    DirectionalLight = nullptr;
    AmbientLight = nullptr;
    for (const AActor* Actor : InWorld->GetActors())
    {
        for (UActorComponent* Component : Actor->GetComponents())
        {
            if (auto* Point = Cast<UPointLightComponent>(Component))
                AllPointLights.Add(Point);
            else if (auto* Spot = Cast<USpotLightComponent>(Component))
                AllSpotLights.Add(Spot);
            else if (auto* Dir = Cast<UDirectionalLightComponent>(Component))
                DirectionalLight = Dir;
            else if (auto* Ambient = Cast<UAmbientLightComponent>(Component))
                AmbientLight = Ambient;
        }
    }
}
void FLightManager::CullLights(const FFrustum& ViewFrustum)
{
    VisiblePointLights.Empty();
    VisibleSpotLights.Empty();

    for (auto* Light : AllPointLights)
    {
        if (ViewFrustum.IntersectsSphere(Light->GetComponentLocation(), Light->GetRadius()))
            VisiblePointLights.Add(Light);
    }

    for (auto* Light : AllSpotLights)
    {
        if (IsSpotLightInFrustum(Light, ViewFrustum))
            VisibleSpotLights.Add(Light);
    }
}
void FLightManager::UploadLightConstants()
{
    FLightingConstants Constants = {};
    FRenderResourceManager* renderResourceManager = GEngine->renderer.GetResourceManager();

    for (int i = 0; i < VisiblePointLights.Num(); ++i)
    {
        auto* L = VisiblePointLights[i];
        Constants.PointLights[i].Color = L->GetLightColor();
        Constants.PointLights[i].Intensity = L->GetIntensity();
        Constants.PointLights[i].Position = L->GetComponentLocation();
        Constants.PointLights[i].Radius = L->GetRadius();
        Constants.PointLights[i].AttenuationFalloff = L->GetAttenuationFalloff();
    }

    for (int i = 0; i < VisibleSpotLights.Num(); ++i)
    {
        auto* L = VisibleSpotLights[i];
        Constants.SpotLights[i].Color = L->GetLightColor();
        Constants.SpotLights[i].Intensity = L->GetIntensity();
        Constants.SpotLights[i].Position = L->GetComponentLocation();
        Constants.SpotLights[i].Direction = L->GetOwner()->GetActorForwardVector();
        Constants.SpotLights[i].InnerAngle = L->GetInnerConeAngle();
        Constants.SpotLights[i].OuterAngle = L->GetOuterConeAngle();
        Constants.SpotLights[i].Radius = L->GetRadius();
        Constants.SpotLights[i].AttenuationFalloff = L->GetAttenuationFalloff();
    }
    if (DirectionalLight) {
        Constants.DirLight.Color = DirectionalLight->GetLightColor();
        Constants.DirLight.Intensity = DirectionalLight->GetIntensity();
        Constants.DirLight.Direction = DirectionalLight->GetOwner()->GetActorForwardVector();
    }
    if (AmbientLight)
    {
        Constants.AmbientLight.Color = AmbientLight->GetLightColor();
        Constants.AmbientLight.Intensity = AmbientLight->GetIntensity();
    }

    Constants.NumPointLights = VisiblePointLights.Num();
    Constants.NumSpotLights = VisibleSpotLights.Num();

    renderResourceManager->UpdateConstantBuffer(TEXT("FLightingConstants"), &Constants);
}
bool FLightManager::IsSpotLightInFrustum(USpotLightComponent* SpotLightComp, const FFrustum& CameraFrustum) const
{
    FVector Apex = SpotLightComp->GetComponentLocation();
    FVector Dir = SpotLightComp->GetOwner()->GetActorForwardVector().Normalize();
    float Range = SpotLightComp->GetRadius();
    float OuterAngleRad = SpotLightComp->GetOuterConeAngle();

    FVector BaseCenter = Apex + Dir * Range;
    float BaseRadius = Range * FMath::Tan(OuterAngleRad);

    if (CameraFrustum.IntersectsPoint(Apex) || CameraFrustum.IntersectsPoint(BaseCenter))
        return true;

    const int SampleCount = 8;
    FVector Right = Dir.Cross(FVector(0, 1, 0));
    if (Right.IsNearlyZero()) Right = Dir.Cross(FVector(1, 0, 0));
    Right.Normalize();
    FVector Up = Dir.Cross(Right).Normalize();

    for (int i = 0; i < SampleCount; ++i)
    {
        float Angle = (2.f * PI * i) / SampleCount;
        FVector Offset = (Right * FMath::Cos(Angle) + Up * FMath::Sin(Angle)) * BaseRadius;
        FVector SamplePoint = BaseCenter + Offset;

        if (CameraFrustum.IntersectsPoint(SamplePoint))
            return true;
    }
    return false;
}