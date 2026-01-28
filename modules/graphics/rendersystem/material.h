#pragma once
#include "../../headeronly/globaltypes.h"
#include "../../headeronly/mathematics.h"
#include "rstypes.h"

// ==================== CULL MODE ====================
enum class CullMode : UINT32
{
    NONE = 0,         // Same as DOUBLE_SIDED
    BACK = 1,         // Cull back faces (default)
    FRONT = 2,        // Cull front faces
    DOUBLE_SIDED = 3  // No culling (show both faces)
};

// ==================== MATERIAL FLAGS ====================
enum class MaterialFlags : UINT32
{
    NONE            = 0,
    ALBEDO_MAP      = 1 << 0,
    NORMAL_MAP      = 1 << 1,
    METALLIC_MAP    = 1 << 2,
    ROUGHNESS_MAP   = 1 << 3,
    AO_MAP          = 1 << 4,
    EMISSIVE_MAP    = 1 << 5,
    ALPHA_BLEND     = 1 << 6,
    CAST_SHADOWS    = 1 << 7,
    RECEIVE_SHADOWS = 1 << 8
};

// Operator overloads for MaterialFlags
inline MaterialFlags operator|(MaterialFlags a, MaterialFlags b)
{
    return static_cast<MaterialFlags>(static_cast<UINT32>(a) | static_cast<UINT32>(b));
}

inline MaterialFlags operator&(MaterialFlags a, MaterialFlags b)
{
    return static_cast<MaterialFlags>(static_cast<UINT32>(a) & static_cast<UINT32>(b));
}

inline MaterialFlags& operator|=(MaterialFlags& a, MaterialFlags b)
{
    return a = a | b;
}

inline MaterialFlags& operator&=(MaterialFlags& a, MaterialFlags b)
{
    return a = a & b;
}

inline MaterialFlags operator~(MaterialFlags a)
{
    return static_cast<MaterialFlags>(~static_cast<UINT32>(a));
}

// ==================== MATERIAL DATA ====================
struct MaterialData
{
    // PBR Properties
    Quark::Color albedo = Quark::Color(0.8f, 0.8f, 0.8f, 1.0f);
    
    // Material params
    float metallic = 0.0f;
    float roughness = 0.5f;
    float ao = 1.0f;
    float alphaCutoff = 0.5f;
    
    // Cull mode + Advanced Properties
    UINT32 cullMode = static_cast<UINT32>(CullMode::BACK);
    
    // Repurposing padding for advanced PBR
    float anisotropy = 0.0f;        // Anisotropic highlights (0-1)
    float clearCoat = 0.0f;         // Clear coat layer intensity (0-1)
    float clearCoatRoughness = 0.0f;// Clear coat roughness (0-1)
    
    // Emissive
    Quark::Color emissive = Quark::Color::Black();
    float emissiveStrength = 0.0f;
    
    // Texture handles
    hTexture albedoMap = 0;
    hTexture normalMap = 0;
    hTexture metallicMap = 0;
    hTexture roughnessMap = 0;
    
    // More handles + flags + padding
    hTexture aoMap = 0;
    hTexture emissiveMap = 0;
    UINT32 flags = 0;
    UINT32 _pad1[4];
};
