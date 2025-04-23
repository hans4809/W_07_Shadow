#pragma once
class UWorld;
struct FVector;

class SceneBuilder
{
public:
    SceneBuilder() = default;
    ~SceneBuilder() = default;
    static void SpawnPointLightGrid(UWorld* World, int CountX, int CountY, int CountZ, FVector StartOffset, float Spacing);
    static void SpawnAppleGrid(UWorld* World, int CountX, int CountY, int CountZ, FVector StartOffset, float Spacing);

    static void SpawnAppleScene(UWorld* World);

    static void SpawnRoomWithSpotLights(UWorld* World, float RoomSize = 500.0f, float WallThickness = 10.0f);

};
