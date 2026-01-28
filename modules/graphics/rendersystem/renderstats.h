#pragma once
#include "../../headeronly/globaltypes.h"

// ==================== RENDER STATISTICS ====================
struct RenderStats
{
    UINT32 drawCalls;
    UINT32 trianglesRendered;
    UINT32 objectsRendered;
    UINT32 objectsCulled;
    UINT32 shadowMapDrawCalls;
    float frameTime;
    float cpuTime;
    float gpuTime;
};
