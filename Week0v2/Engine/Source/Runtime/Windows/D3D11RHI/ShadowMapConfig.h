#pragma once
#include "HAL/PlatformType.h"

namespace Shadow
{
    static const uint32 CASCADE_COUNT = 4;
    static const uint32 CASCADE_RES   = 1024;                // 한 캐스케이드당 해상도
    static const uint32 ATLAS_COLS    = 2;                   // √4 = 2
    static const uint32 ATLAS_ROWS    = 2;

    // 전체 아틀라스 크기
    constexpr uint32_t AtlasWidth  = ATLAS_COLS * CASCADE_RES;
    constexpr uint32_t AtlasHeight = ATLAS_ROWS * CASCADE_RES;
}
