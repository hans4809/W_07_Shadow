#include "ShaderHeaders/UberLitCommon.hlsli"

static const uint TILE_SIZE = 16;
static const uint BIT_PARTITIONS = 32;

Texture2D<float> DepthView : register(t0);
RWStructuredBuffer<uint> TileDepthMask : register(u0);

cbuffer CullingCB : register(b8)
{
    uint ScreenWidth; // 렌더 타겟 가로 픽셀 수
    uint ScreenHeight; // 렌더 타겟 세로 픽셀 수
    uint numPointLights; // PointLight 개수
    uint numSpotLights; // SpotLight 개수
    uint maxLightsPerTile; // 타일당 최대 라이트 수
    float nearClip; // 뷰 공간 Near
    float farClip; // 뷰 공간 Far
};

cbuffer CameraCB : register(b9)
{
    row_major matrix view;
    row_major matrix viewProj;
    row_major matrix invViewProj;
};
