#include "SceneBuilder.h"

#include "FLoaderOBJ.h"
#include "StaticMeshActor.h"
#include "World.h"
#include "Actors/PointLightActor.h"
#include "Components/LightComponents/PointLightComponent.h"
#include "Components/PrimitiveComponents/MeshComponents/StaticMeshComponents/StaticMeshComponent.h"

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
    int c = 3;
    SpawnAppleGrid(World, 10, 10, 10, FVector(0.0f, 0.0f, 0.0f), 1.0f);
    SpawnPointLightGrid(World, c, c, c, FVector(0.5f, 0.5f, 0.5f), 1.0f);
}