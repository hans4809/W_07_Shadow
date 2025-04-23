#include "ShaderHeaders/CullingHeader.hlsli"

[numthreads(1, 1, 1)]
void mainCS(uint3 dispatchId : SV_DispatchThreadID)
{
    uint tx = dispatchId.x;
    uint ty = dispatchId.y;
    uint tilesX = (ScreenWidth + TILE_SIZE - 1) / TILE_SIZE;
    uint tileIndex = ty * tilesX + tx;

    uint depthMask = TileDepthMask[tileIndex];
    float2 tileMin = float2(tx * TILE_SIZE, ty * TILE_SIZE);
    float2 tileMax = tileMin + float2(TILE_SIZE, TILE_SIZE);
    uint count = 0;

    // --- PointLights 컬링 ---
    for (uint i = 0; i < numPointLights; ++i)
    {
        FPointLight L = PointLightBuffer[i];
        float3 lightViewPos = mul(float4(L.Position, 1), view).xyz;
        // Depth Bitmask 테스트
        float zMin = max(nearClip, lightViewPos.z - L.Radius);
        float zMax = min(farClip, lightViewPos.z + L.Radius);
        uint bMin = min((uint) (((zMin - nearClip) / (farClip - nearClip)) * BIT_PARTITIONS), BIT_PARTITIONS - 1);
        uint bMax = min((uint) (((zMax - nearClip) / (farClip - nearClip)) * BIT_PARTITIONS), BIT_PARTITIONS - 1);
        uint lightMask = (~0u << bMin) & ((bMax == BIT_PARTITIONS - 1) ? ~0u : ((1u << (bMax + 1)) - 1));
        if ((depthMask & lightMask) == 0)
            continue;

        // 화면 공간 투영
        float4 cp = mul(float4(L.Position, 1), viewProj);
        float2 ndc = cp.xy / cp.w;
        float2 screenPos = ndc * 0.5 + 0.5;
        screenPos *= float2(ScreenWidth, ScreenHeight);

        // AABB-구 테스트
        float2 closest = clamp(screenPos, tileMin, tileMax);
        float dist2 = dot(closest - screenPos, closest - screenPos);
        if (dist2 > L.Radius * L.Radius)
            continue;

        // 결과 기록
        uint writeIdx = tileIndex * maxLightsPerTile + count;
        LightIndexList[writeIdx] = i;
        ++count;
        if (count >= maxLightsPerTile)
            break;
    }

    // --- SpotLights 컬링 ---
    for (uint i = 0; i < numSpotLights; ++i)
    {
        FSpotLight L = SpotLightBuffer[i];
        float3 lightViewPos = mul(float4(L.Position, 1), view).xyz;
        // Depth Bitmask 테스트
        float zMin = max(nearClip, lightViewPos.z - L.Radius);
        float zMax = min(farClip, lightViewPos.z + L.Radius);
        uint bMin = min((uint) (((zMin - nearClip) / (farClip - nearClip)) * BIT_PARTITIONS), BIT_PARTITIONS - 1);
        uint bMax = min((uint) (((zMax - nearClip) / (farClip - nearClip)) * BIT_PARTITIONS), BIT_PARTITIONS - 1);
        uint lightMask = (~0u << bMin) & ((bMax == BIT_PARTITIONS - 1) ? ~0u : ((1u << (bMax + 1)) - 1));
        if ((depthMask & lightMask) == 0)
            continue;

        // 화면 공간 투영
        float4 cp = mul(float4(L.Position, 1), viewProj);
        float2 ndc = cp.xy / cp.w;
        float2 screenPos = ndc * 0.5 + 0.5;
        screenPos *= float2(ScreenWidth, ScreenHeight);

        // AABB-구 기본 테스트
        float2 closest = clamp(screenPos, tileMin, tileMax);
        float dist2 = dot(closest - screenPos, closest - screenPos);
        if (dist2 > L.Radius * L.Radius)
            continue;

        // 콘 방향 검사
        float2 uv = ((tileMin + tileMax) * 0.5 / float2(ScreenWidth, ScreenHeight)) * 2.0 - 1.0;
        float4 proj = float4(uv * cp.w, cp.z, cp.w);
        float3 viewPos = mul(invViewProj, proj).xyz;
        float3 toTile = normalize(viewPos - lightViewPos);
        float3 lightViewDir = mul(float4(L.Position, 0), view).xyz;
        float3 normalizedLightViewDir = normalize(toTile - lightViewDir);

        float cosOuter = cos(L.OuterAngle);
        if (dot(toTile, normalizedLightViewDir) < cosOuter)
            continue;

        // 결과 기록
        uint writeIdx = tileIndex * maxLightsPerTile + count;
        LightIndexList[writeIdx] = numPointLights + i;
        ++count;
        if (count >= maxLightsPerTile)
            break;
    }

    LightCount[tileIndex] = count;
}
