#include "LightManager.h"

#include "EditorEngine.h"
#include "Components/LightComponents/AmbientLightComponent.h"
#include "D3D11RHI/CBStructDefine.h"
#include "Math/JungleMath.h"
#include "Renderer/RenderResourceManager.h"
#include "UnrealEd/PrimitiveBatch.h"
#include "Components/LightComponents/PointLightComponent.h"
#include "Components/LightComponents/SpotLightComponent.h"
#include "Components/LightComponents/DirectionalLightComponent.h"
void FLightManager::CollectLights(const UWorld* InWorld)
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


    FRenderer& Renderer = GEngine->renderer;
    FRenderResourceManager* renderResourceManager = Renderer.GetResourceManager();
    int PointLightID = 0;
    for (auto* Light : AllPointLights)
    {
        if (ViewFrustum.IntersectsSphere(Light->GetComponentLocation(), Light->GetRadius()))
        {
            if (VisiblePointLights.Num() < MAX_POINT_LIGHTS) {
                VisiblePointLights.Add(Light);

                Light->ShadowSRVSlice.Empty();
                for (int i = 0; i < 6; i++)
                {
                    if (auto a = renderResourceManager->GetShadowMapSliceSRVs(TEXT("PointLightShadowMap"), PointLightID * 6 + i))
                    {
                        Light->ShadowSRVSlice.Add(a);
                    }
                }
                ++PointLightID;
            }
        }
    }

    int SpotLightID = 0;
    for (auto* Light : AllSpotLights)
    {
        if (IsSpotLightInFrustum(Light, ViewFrustum))
        {
            if (VisibleSpotLights.Num() < MAX_SPOT_LIGHTS) {
                VisibleSpotLights.Add(Light);

                Light->ShadowSRVSlice.Empty();
                Light->ShadowSRVSlice.Add(
                    renderResourceManager->GetShadowMapSliceSRVs(TEXT("SpotLightShadowMap"), SpotLightID)
                );

                ++SpotLightID;
            }
        }
    }

    if (DirectionalLight)
    {
        DirectionalLight->ShadowSRVSlice.Empty();
        for (int i = 0; i < MAX_CASCADES; i++)
        {
            DirectionalLight->ShadowSRVSlice.Add(
                renderResourceManager->GetShadowMapSliceSRVs(TEXT("DirLightShadowMap"), i)
            );
        }
    }
}
void FLightManager::UploadLightConstants()
{
    FLightingConstants Constants = {};
    FRenderResourceManager* renderResourceManager = GEngine->renderer.GetResourceManager();

    for (int i = 0; i < std::min(VisiblePointLights.Num(), MAX_POINT_LIGHTS); ++i)
    {
        const UPointLightComponent* L = VisiblePointLights[i];
        Constants.PointLights[i].Color = L->GetLightColor();
        Constants.PointLights[i].Intensity = L->GetIntensity();
        Constants.PointLights[i].Position = L->GetComponentLocation();
        Constants.PointLights[i].Radius = L->GetRadius();
        Constants.PointLights[i].AttenuationFalloff = L->GetAttenuationFalloff();
        Constants.PointLights[i].bCastShadow = L->CanCastShadows();
    }

    for (int i = 0; i < std::min(VisibleSpotLights.Num(),MAX_SPOT_LIGHTS); ++i)
    {
        const USpotLightComponent* L = VisibleSpotLights[i];
        Constants.SpotLights[i].Color = L->GetLightColor();
        Constants.SpotLights[i].Intensity = L->GetIntensity();
        Constants.SpotLights[i].Position = L->GetComponentLocation();
        Constants.SpotLights[i].Direction = L->GetOwner()->GetActorForwardVector();
        Constants.SpotLights[i].InnerAngle = L->GetInnerConeAngle();
        Constants.SpotLights[i].OuterAngle = L->GetOuterConeAngle();
        Constants.SpotLights[i].Radius = L->GetRadius();
        Constants.SpotLights[i].AttenuationFalloff = L->GetAttenuationFalloff();
        Constants.SpotLights[i].bCastShadow = L->CanCastShadows();
    }
    if (DirectionalLight)
    {
        Constants.DirLight.Color = DirectionalLight->GetLightColor();
        Constants.DirLight.Intensity = DirectionalLight->GetIntensity();
        Constants.DirLight.Direction = DirectionalLight->GetOwner()->GetActorForwardVector();
        for (int i = 0; i < MAX_CASCADES; i++)
            Constants.DirLight.ViewProjectionMatrix[i] = DirectionalLight->GetViewProjectionMatrix(i);
        for (int i = 0; i < MAX_CASCADES + 1; i++)
            Constants.DirLight.CascadeSplits[i] = DirectionalLight->GetCascadeSplits()[i];
        Constants.DirLight.bCastShadow = DirectionalLight->CanCastShadows();
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
bool FLightManager::IsSpotLightInFrustum(const USpotLightComponent* SpotLightComp, const FFrustum& CameraFrustum)
{
    const FVector Apex = SpotLightComp->GetComponentLocation();
    const FVector Dir = SpotLightComp->GetOwner()->GetActorForwardVector().Normalize();
    const float Range = SpotLightComp->GetRadius();
    const float OuterAngleRad = SpotLightComp->GetOuterConeAngle();

    const FVector BaseCenter = Apex + Dir * Range;
    const float BaseRadius = Range * FMath::Tan(OuterAngleRad);

    if (CameraFrustum.IntersectsPoint(Apex) || CameraFrustum.IntersectsPoint(BaseCenter))
        return true;

    constexpr int SampleCount = 8;
    FVector Right = Dir.Cross(FVector(0, 1, 0));
    if (Right.IsNearlyZero()) Right = Dir.Cross(FVector(1, 0, 0));
    const FVector normalizedRight = Right.Normalize();
    const FVector Up = Dir.Cross(normalizedRight).Normalize();

    for (int i = 0; i < SampleCount; ++i)
    {
        const float Angle = (2.f * PI * i) / SampleCount;
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

    for (const USpotLightComponent* Spot : VisibleSpotLights)
    {
        const float Length = Spot->GetRadius();
        const FVector Pos = Spot->GetComponentLocation();
        const FMatrix Model = JungleMath::CreateModelMatrix(/*Pos*/{}, Spot->GetComponentRotation(), Spot->GetComponentScale());
        const FVector4 Color = Spot->GetLightColor();

        const float OuterR = tan(Spot->GetOuterConeAngle()) * Length;
        const float InnerR = tan(Spot->GetInnerConeAngle()) * Length;

        if (Spot->GetOuterConeAngle() > 0)
            Batch.AddCone(Pos, OuterR, Length, 15, Color, Model);
        if (Spot->GetInnerConeAngle() > 0)
            Batch.AddCone(Pos, InnerR, Length, 15, Color, Model);
    }
    if (DirectionalLight) 
    {
        const FVector Origin = DirectionalLight->GetComponentLocation();
        const FVector Forward = DirectionalLight->GetOwner()->GetActorForwardVector();
        const FVector Right = DirectionalLight->GetOwner()->GetActorRightVector();
        const FVector4 Color = DirectionalLight->GetLightColor();

        for (int i = 0; i < 4; ++i)
        {
            Batch.AddLine(Origin + Right * (-1.5f + i), Forward, 15.0f, Color);
        }
    }

    for (const UPointLightComponent* Point : VisiblePointLights)
    {
        if (Point->GetRadius() > 0)
        {
            Batch.AddSphere(Point->GetComponentLocation(), Point->GetRadius(), Point->GetLightColor());
        }
    }
}

uint32 FLightManager::GetAmbientLightNum() const
{
    if (AmbientLight) return 1; 
    else return 0;
}

uint32 FLightManager::GetDirectionalLightNum() const
{
    if (DirectionalLight) return 1;
    else return 0;
}

uint32 FLightManager::GetPointLightNum() const
{
    return AllPointLights.Num();
}

uint32 FLightManager::GetSpotLightNum() const
{
    return AllSpotLights.Num();
}
