#include "SceneBuilder.h"

#include "FLoaderOBJ.h"
#include "StaticMeshActor.h"
#include "World.h"
#include "Actors/PointLightActor.h"
#include "Actors/SpotLightActor.h"
#include "Components/LightComponents/PointLightComponent.h"
#include "Components/LightComponents/SpotLightComponent.h"
#include "Components/PrimitiveComponents/MeshComponents/StaticMeshComponents/StaticMeshComponent.h"
#include "Math/JungleMath.h"

void SceneBuilder::SpawnAppleGrid(UWorld* World, int CountX, int CountY, int CountZ, FVector StartOffset, float Spacing)
{
    if (!World) return;

    for (int x = 0; x < CountX; ++x)
        for (int y = 0; y < CountY; ++y)
            for (int z = 0; z < CountZ; ++z)
            {
                FVector Pos = StartOffset + FVector(x * Spacing, y * Spacing, z * Spacing);

                AStaticMeshActor* TempActor = World->SpawnActor<AStaticMeshActor>();
                TempActor->SetActorLabel(TEXT("OBJ_SPHERE"));

                UStaticMeshComponent* MeshComp = TempActor->GetStaticMeshComponent();
                FManagerOBJ::CreateStaticMesh("Assets/apple_mid.obj"); // ensure it exists
                MeshComp->SetStaticMesh(FManagerOBJ::GetStaticMesh(L"apple_mid.obj"));

                TempActor->SetActorLocation(Pos);
            }
}
void SceneBuilder::SpawnPointLightGrid(UWorld* World, int CountX, int CountY, int CountZ, FVector StartOffset, float Spacing)
{
    if (!World) return;

    for (int x = 0; x < CountX; ++x)
        for (int y = 0; y < CountY; ++y)
            for (int z = 0; z < CountZ; ++z)
            {
                FVector Pos = StartOffset + FVector(x * Spacing, y * Spacing, z * Spacing);

                APointLightActor* LightActor = World->SpawnActor<APointLightActor>();
                LightActor->SetActorLabel(TEXT("PointLight"));

                /*UPointLightComponent* LightComp = LightActor->GetPointLightComponent();
                LightComp->SetIntensity(5000.0f);
                LightComp->SetRadius(300.0f);*/

                LightActor->SetActorLocation(Pos);
            }
}
void SceneBuilder::SpawnAppleScene(UWorld* World)
{
    if (!World) return;
    /*
    int a = 5;
    int c = 6;
    SpawnAppleGrid(World, a, a, a, FVector(0.0f, 0.0f, 0.0f), 25.0f);
    SpawnPointLightGrid(World, c, c, c, FVector(0.5f, 0.5f, 0.5f), 25.0f);
*/
    SpawnRoomWithSpotLights(World, 20, 0.5f);

}
void SceneBuilder::SpawnRoomWithSpotLights(UWorld* World, float RoomSize, float WallThickness)
{
    if (!World) return;

    const FVector RoomCenter(0.0f, 0.0f, 0.0f);
    const float Half = RoomSize;

    auto SpawnCubeAt = [&](FVector Location, FVector Scale, const FString& Label)
        {
            AStaticMeshActor* TempActor = World->SpawnActor<AStaticMeshActor>();
            TempActor->SetActorLabel(*Label);
            UStaticMeshComponent* MeshComp = TempActor->GetStaticMeshComponent();
            FManagerOBJ::CreateStaticMesh("Assets/Cube.obj");
            MeshComp->SetStaticMesh(FManagerOBJ::GetStaticMesh(L"Cube.obj"));
            TempActor->SetActorLocation(Location);
            TempActor->SetActorScale(Scale);
        };

    // 바닥
    SpawnCubeAt(RoomCenter - FVector(0, 0, Half), FVector(RoomSize, RoomSize, WallThickness), TEXT("Floor"));

    // 천장
    SpawnCubeAt(RoomCenter + FVector(0, 0, Half), FVector(RoomSize, RoomSize, WallThickness), TEXT("Ceiling"));

    // 벽 4개
// 벽 4개 (중심 기준으로 벽의 절반 두께만큼 안쪽으로 이동)
    SpawnCubeAt(RoomCenter + FVector(Half - WallThickness * 0.5f, 0, 0), FVector(WallThickness, RoomSize, RoomSize), TEXT("Wall+X"));
    SpawnCubeAt(RoomCenter - FVector(Half - WallThickness * 0.5f, 0, 0), FVector(WallThickness, RoomSize, RoomSize), TEXT("Wall-X"));
    SpawnCubeAt(RoomCenter + FVector(0, Half - WallThickness * 0.5f, 0), FVector(RoomSize, WallThickness, RoomSize), TEXT("Wall+Y"));
    SpawnCubeAt(RoomCenter - FVector(0, Half - WallThickness * 0.5f, 0), FVector(RoomSize, WallThickness, RoomSize), TEXT("Wall-Y"));
    const FVector Center = FVector(0, 0, 0); // RoomCenter
    const float H = RoomSize;
    const float T = WallThickness * 1.5f;
    APointLightActor* LightActor = World->SpawnActor<APointLightActor>();
    LightActor->SetActorLabel(FString::Printf(TEXT("CenterLight")));

    UPointLightComponent* LightComp = static_cast<UPointLightComponent*>(LightActor->GetLightComponent());
        //LightComp->SetRelativeRotation(DirToCenter);
    LightComp->SetIntensity(1.0f);
    LightComp->SetRadius(Half * 2);
    /*const FVector WallCenters[6] =
    {
        Center + FVector(0, 0,  H - T), // 천장
        Center - FVector(0, 0,  H - T), // 바닥
        Center + FVector(H - T, 0, 0), // +X 벽
        Center - FVector(H - T, 0, 0), // -X 벽
        Center + FVector(0,  H - T, 0), // +Y 벽
        Center - FVector(0,  H - T, 0)  // -Y 벽
    };
    for (int i = 0; i < 6; ++i)
    {
        APointLightActor* LightActor = World->SpawnActor<APointLightActor>();
        LightActor->SetActorLabel(FString::Printf(TEXT("WallCenterLight_%d"), i));
        LightActor->SetActorLocation(WallCenters[i]);

        UPointLightComponent* LightComp = static_cast<UPointLightComponent*>(LightActor->GetLightComponent());
        LightComp->SetIntensity(1.0f);
        LightComp->SetRadius(RoomSize); // 혹은 H * 2
    }*/

    // 모서리 Point Light
    const FVector Corners[4] =
    {
        FVector(-Half, -Half, -Half),
        FVector(Half, -Half, -Half),
        FVector(Half,  Half, -Half),
        FVector(-Half,  Half, -Half)
    };

    for (int i = 0; i < 4; ++i)
    {
        /*{
            APointLightActor* LightActor = World->SpawnActor<APointLightActor>();
            LightActor->SetActorLabel(FString::Printf(TEXT("CornerLight_%d"), i));
            LightActor->SetActorLocation(Corners[i] - Corners[i] / Half * 1.0f);

            UPointLightComponent* LightComp = static_cast<UPointLightComponent*>(LightActor->GetLightComponent());
            FVector DirToCenter = (RoomCenter - Corners[i]).Normalize()/PI*180.0f;

            LightComp->GetOwner()->SetActorRotation(DirToCenter);#2#
            //LightComp->SetRelativeRotation(DirToCenter);
            LightComp->SetIntensity(1.0f);
            LightComp->SetRadius(Half * 2);
        }*/
        // 천장 모서리 (Z축 반전)
        /*{
            APointLightActor* LightActor = World->SpawnActor<APointLightActor>();
            LightActor->SetActorLabel(FString::Printf(TEXT("CornerLight_Top_%d"), i));
            FVector CeilingCorner = Corners[i];
            CeilingCorner.z *= -1.0f; // 천장 쪽
            FVector Pos = CeilingCorner - CeilingCorner / Half * 1.0f;
            LightActor->SetActorLocation(Pos);

            UPointLightComponent* LightComp = static_cast<UPointLightComponent*>(LightActor->GetLightComponent());
            LightComp->SetIntensity(5.0f);
            LightComp->SetRadius(Half * 2);
        }*/
    }
}