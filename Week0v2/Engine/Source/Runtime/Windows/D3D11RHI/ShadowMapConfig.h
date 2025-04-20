#pragma once
#include "HAL/PlatformType.h"

namespace Shadow
{
    static constexpr uint32 CASCADE_COUNT = 4;
    static constexpr uint32 CASCADE_RES   = 1024;                // 한 캐스케이드당 해상도
    static constexpr uint32 DIR_ATLAS_COLS    = 2;                   // √4 = 2
    static constexpr uint32 DIR_ATLAS_ROWS    = 2;

    // 전체 아틀라스 크기
    constexpr uint32 DirectionalAtlasWidth  = DIR_ATLAS_COLS * CASCADE_RES;
    constexpr uint32 DirectionalAtlasHeight = DIR_ATLAS_ROWS * CASCADE_RES;

    static constexpr uint32 MAX_SPOT_LIGHT_COUNT = 16;
    static constexpr uint32 SPOT_LIGHT_RES = 1024;
    static constexpr uint32 SPOT_ATLAS_COLS = 4;
    static constexpr uint32 SPOT_ATLAS_ROWS = 4;

    inline uint32 SpotAtlasWidth = SPOT_ATLAS_COLS * SPOT_ATLAS_ROWS;
    inline uint32 SpotAtlasHeight = SPOT_ATLAS_ROWS * SPOT_ATLAS_COLS;
}
