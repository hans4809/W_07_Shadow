#include "LightManager.h"

#include "EditorEngine.h"
#include "Components/LightComponents/AmbientLightComponent.h"
#include "D3D11RHI/CBStructDefine.h"
#include "Math/JungleMath.h"
#include "Renderer/RenderResourceManager.h"
#include "UnrealEd/PrimitiveBatch.h"

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
            if (auto* Spot = Cast<USpotLightComponent>(Component))
                AllSpotLights.Add(Spot);
            else if (auto* Point = Cast<UPointLightComponent>(Component))
                AllPointLights.Add(Point);
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
        for (int i = 0; i < MAX_CASCADES; i++)
            Constants.DirLight.ViewProjectionMatrix[i] = DirectionalLight->GetViewProjectionMatrix(i);
        for (int i = 0; i < MAX_CASCADES + 1; i++)
            Constants.DirLight.CascadeSplits[i] = DirectionalLight->GetCascadeSplits()[i];
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
void FLightManager::VisualizeLights()
{
    UPrimitiveBatch& Batch = UPrimitiveBatch::GetInstance();

    for (auto* Spot : VisibleSpotLights)
    {
        const float Length = Spot->GetRadius();
        const FVector Pos = Spot->GetComponentLocation();
        const FMatrix Model = JungleMath::CreateModelMatrix(/*Pos*/{}, Spot->GetComponentRotation(), Spot->GetComponentScale());
        const FVector4 Color = Spot->GetLightColor();

        float OuterR = tan(Spot->GetOuterConeAngle()) * Length;
        float InnerR = tan(Spot->GetInnerConeAngle()) * Length;

        if (Spot->GetOuterConeAngle() > 0)
            Batch.AddCone(Pos, OuterR, Length, 15, Color, Model);
        if (Spot->GetInnerConeAngle() > 0)
            Batch.AddCone(Pos, InnerR, Length, 15, Color, Model);
    }
    if (DirectionalLight) {
        FVector Origin = DirectionalLight->GetComponentLocation();
        FVector Forward = DirectionalLight->GetOwner()->GetActorForwardVector();
        FVector Right = DirectionalLight->GetOwner()->GetActorRightVector();
        FVector4 Color = DirectionalLight->GetLightColor();

        for (int i = 0; i < 4; ++i)
        {
            Batch.AddLine(Origin + Right * (-1.5f + i), Forward, 15.0f, Color);
        }
    }

    for (auto* Point : VisiblePointLights)
    {
        if (Point->GetRadius() > 0)
        {
            Batch.AddSphere(Point->GetComponentLocation(), Point->GetRadius(), Point->GetLightColor());
        }
    }
}
