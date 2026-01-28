#pragma once
#include "../../headeronly/globaltypes.h"
#include "../../headeronly/mathematics.h"
#include "rstypes.h"

// ==================== RENDER OBJECT FLAGS ====================
enum class RenderObjectFlags : UINT32
{
    NONE           = 0,
    STATIC         = 1 << 0,   // Object transform won't change at runtime
    DYNAMIC        = 1 << 1,   // Object transform may change at runtime
    VISIBLE        = 1 << 2,   // Object should be rendered
    CAST_SHADOW    = 1 << 3,   // Object casts shadows
    RECEIVE_SHADOW = 1 << 4,   // Object receives shadows
    FRUSTUM_CULL   = 1 << 5    // Object participates in frustum culling
};

// Bitwise operators for RenderObjectFlags
inline RenderObjectFlags operator|(RenderObjectFlags a, RenderObjectFlags b)
{
    return static_cast<RenderObjectFlags>(static_cast<UINT32>(a) | static_cast<UINT32>(b));
}

inline RenderObjectFlags operator&(RenderObjectFlags a, RenderObjectFlags b)
{
    return static_cast<RenderObjectFlags>(static_cast<UINT32>(a) & static_cast<UINT32>(b));
}

inline RenderObjectFlags& operator|=(RenderObjectFlags& a, RenderObjectFlags b)
{
    return a = a | b;
}

inline RenderObjectFlags& operator&=(RenderObjectFlags& a, RenderObjectFlags b)
{
    return a = a & b;
}

inline RenderObjectFlags operator~(RenderObjectFlags a)
{
    return static_cast<RenderObjectFlags>(~static_cast<UINT32>(a));
}


// ==================== RENDER OBJECT ====================
struct RenderObject
{
    hMesh mesh = 0;
    hMaterial material = 0;
    Quark::Mat4 worldMatrix;
    Quark::AABB worldAABB;
    RenderObjectFlags flags = RenderObjectFlags::VISIBLE | RenderObjectFlags::FRUSTUM_CULL | 
                              RenderObjectFlags::CAST_SHADOW | RenderObjectFlags::RECEIVE_SHADOW;
};

