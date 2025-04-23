#include "ShaderHeaders/CullingHeader.hlsli"

groupshared uint sharedMask;

//===============================================
// 1) Tile Depth Mask 생성
//    raw depth(0~1) -> view-space 거리로 선형화 -> 비트마스크
//===============================================
[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void mainCS(uint3 dispatchID : SV_DispatchThreadID, uint3 groupThreadID : SV_GroupThreadID, uint3 groupID : SV_GroupID)
{
    if (groupThreadID.x == 0 && groupThreadID.y == 0)
        sharedMask = 0;
    GroupMemoryBarrierWithGroupSync();

    uint2 pix = dispatchID.xy;
    if (pix.x < ScreenWidth && pix.y < ScreenHeight)
    {
        // 하드웨어 깊이(raw depth) 읽기
        float raw = DepthView.Load(int3(pix.xy, 0));
        // 선형화된 뷰 공간 거리 계산
        float viewZ = (nearClip * farClip) / (farClip - raw * (farClip - nearClip));
        // 0~1 정규화
        float normZ = saturate((viewZ - nearClip) / (farClip - nearClip));
        // 비트 인덱스
        uint bit = min((uint) (normZ * BIT_PARTITIONS), BIT_PARTITIONS - 1);
        // 그룹 공유 마스크에 원자 OR
        InterlockedOr(sharedMask, 1u << bit, sharedMask);
    }
    GroupMemoryBarrierWithGroupSync();

    if (groupThreadID.x == 0 && groupThreadID.y == 0)
    {
        uint tilesX = (ScreenWidth + TILE_SIZE - 1) / TILE_SIZE;
        uint tileIndex = groupID.y * tilesX + groupID.x;
        TileDepthMask[tileIndex] = sharedMask;
    }
}