#pragma once
#include "../../headeronly/globaltypes.h"
#include "../../headeronly/mathematics.h"

// LIMITS & CONSTANTS
constexpr UINT32 MAX_LIGHTS = 64;
constexpr UINT32 MAX_DIRECTIONAL_LIGHTS = 1;
constexpr UINT32 MAX_SPOT_LIGHTS = 48;
constexpr UINT32 MAX_POINT_LIGHTS = 15;

constexpr UINT32 MAX_SPOT_SHADOW_LIGHTS = 16;
constexpr UINT32 MAX_POINT_SHADOW_LIGHTS = 5;

constexpr UINT32 DIRECTIONAL_CASCADE_COUNT = 4;
constexpr UINT32 POINT_SHADOW_FACE_COUNT = 6;  // Cube map has 6 faces

constexpr float DIRECTIONAL_SHADOW_ATLAS_SIZE = 4096.0f;
constexpr float LOCAL_LIGHT_SHADOW_ATLAS_SIZE = 4096.0f;
constexpr UINT32 LOCAL_SHADOW_GRID_SIZE = 8;  // 8x8 grid = 64 slots
constexpr UINT32 LOCAL_SHADOW_SLOT_SIZE = 512; // 4096 / 8 = 512px per slot

// ENUMS
enum class LightType : UINT32
{
    NONE = 0,
    DIRECTIONAL = 1,
    POINT = 2,
    SPOT = 3
};

enum class ShadowQuality : UINT32
{
    NONE = 0,
    HARD = 1,
    MEDIUM = 2,
    SOFT = 3
};

enum class LightFlags : UINT32
{
    NONE = 0,
    LIGHT_ENABLED = 1<<0,
    LIGHT_CAST_SHADOWS = 1<<1
};

inline LightFlags operator|(LightFlags a, LightFlags b)
{
    return static_cast<LightFlags>(static_cast<UINT32>(a) | static_cast<UINT32>(b));
}

inline LightFlags operator&(LightFlags a, LightFlags b)
{
    return static_cast<LightFlags>(static_cast<UINT32>(a) & static_cast<UINT32>(b));
}

inline LightFlags operator^(LightFlags a, LightFlags b)
{
    return static_cast<LightFlags>(static_cast<UINT32>(a) ^ static_cast<UINT32>(b));
}

inline LightFlags operator~(LightFlags a)
{
    return static_cast<LightFlags>(~static_cast<UINT32>(a));
}

inline LightFlags& operator|=(LightFlags& a, LightFlags b)
{
    a = a | b;
    return a;
}

inline LightFlags& operator&=(LightFlags& a, LightFlags b)
{
    a = a & b;
    return a;
}

inline LightFlags& operator^=(LightFlags& a, LightFlags b)
{
    a = a ^ b;
    return a;
}

struct GPULightData
{
    Quark::Vec3 position;
    UINT32 type;

    Quark::Vec3 direction;
    float range;

    Quark::Vec3 color;
    float intensity;

    Quark::Vec2 spotAngles;
    UINT32 shadowQuality;  // 0=None, 1=Hard(1x1), 2=Medium(3x3), 3=Soft(5x5)
    UINT32 flags;

    Quark::Vec3 attenuation;
    int shadowIndex;  // Slot index in local shadow atlas (-1 = no shadow)

    Quark::Vec4 cascadeSplits;
    Quark::Mat4 cascadeMatrices[DIRECTIONAL_CASCADE_COUNT];
    
    Quark::Mat4 spotShadowMatrix;  // Spot light shadow viewProj matrix
    Quark::Mat4 pointShadowMatrices[POINT_SHADOW_FACE_COUNT];  // Point light cube face matrices
};

struct DirectionalLight
{
    Quark::Vec3 direction = Quark::Vec3(0, -1, 0);
    Quark::Color color = Quark::Color::White();
    float intensity = 1.0f;

    UINT32 flags = static_cast<UINT32>(LightFlags::LIGHT_ENABLED | LightFlags::LIGHT_CAST_SHADOWS);
    ShadowQuality shadowQuality = ShadowQuality::SOFT;

    GPULightData toGPU() const
    {
        GPULightData gpu{};
        gpu.type = static_cast<UINT32>(LightType::DIRECTIONAL);
        gpu.direction = direction.Normalized();
        gpu.color = { color.r, color.g, color.b };
        gpu.intensity = intensity;
        gpu.flags = flags;

        gpu.shadowQuality = static_cast<UINT32>(shadowQuality);
        gpu.shadowIndex = (flags & static_cast<UINT32>(LightFlags::LIGHT_CAST_SHADOWS)) ? 0 : -1;
        gpu.attenuation = Quark::Vec3(1.0f, 0.0f, 0.0f);

        return gpu;
    }
};

struct PointLight
{
    Quark::Vec3 position = Quark::Vec3::Zero();
    Quark::Color color = Quark::Color::White();
    float intensity = 1.0f;
    float range = 10.0f;

    Quark::Vec3 attenuation = { 1.0f, 0.09f, 0.032f };
    UINT32 flags = static_cast<UINT32>(LightFlags::LIGHT_ENABLED);
    ShadowQuality shadowQuality = ShadowQuality::SOFT;

    GPULightData toGPU() const
    {
        GPULightData gpu{};
        gpu.type = static_cast<UINT32>(LightType::POINT);
        gpu.position = position;
        gpu.range = range;
        gpu.color = { color.r, color.g, color.b };
        gpu.intensity = intensity;
        gpu.flags = flags;

        gpu.shadowQuality = static_cast<UINT32>(shadowQuality);
        gpu.attenuation = attenuation;
        gpu.shadowIndex = (flags & static_cast<UINT32>(LightFlags::LIGHT_CAST_SHADOWS)) ? 0 : -1;

        return gpu;
    }
};

struct SpotLight
{
    Quark::Vec3 position = Quark::Vec3::Zero();
    Quark::Vec3 direction = Quark::Vec3(0, -1, 0);
    Quark::Color color = Quark::Color::White();
    float intensity = 1.0f;
    float range = 10.0f;

    float innerCutoff = 0.9f;
    float outerCutoff = 0.8f;

    UINT32 flags = static_cast<UINT32>(LightFlags::LIGHT_ENABLED);
    ShadowQuality shadowQuality = ShadowQuality::SOFT;

    GPULightData toGPU() const
    {
        GPULightData gpu{};
        gpu.type = static_cast<UINT32>(LightType::SPOT);
        gpu.position = position;
        gpu.direction = direction.Normalized();
        gpu.range = range;
        gpu.color = { color.r, color.g, color.b };
        gpu.intensity = intensity;
        gpu.spotAngles = { innerCutoff, outerCutoff };
        gpu.flags = flags;

        gpu.shadowQuality = static_cast<UINT32>(shadowQuality);
        gpu.attenuation = { 1.0f, 0.09f, 0.032f };
        gpu.shadowIndex = -1;  // Will be set by FramePacketBuilder if shadow enabled

        return gpu;
    }
};

// ==================== SPOT SHADOW MATRIX CALCULATION ====================
inline Quark::Mat4 computeSpotShadowMatrix(const SpotLight& light)
{
    // 1. Determine up vector (avoid parallel with direction)
    Quark::Vec3 dir = light.direction.Normalized();
    Quark::Vec3 up = (std::abs(dir.y) > 0.99f) 
        ? Quark::Vec3(1.0f, 0.0f, 0.0f) 
        : Quark::Vec3(0.0f, 1.0f, 0.0f);
    
    // 2. Build view matrix: look from position toward direction
    Quark::Mat4 view = Quark::Mat4::LookAt(
        light.position,
        light.position + dir,
        up
    );
    
    // 3. Build perspective projection
    // FOV = 2 * acos(outerCutoff) - outer cutoff is cos of half-angle
    float halfAngle = std::acos(light.outerCutoff);
    float fov = halfAngle * 2.0f * 1.1f;  // 10% padding for smooth edges
    
    Quark::Mat4 proj = Quark::Mat4::Perspective(
        fov,
        1.0f,           // Aspect 1:1 (square shadow map slot)
        0.1f,           // Near plane
        light.range     // Far plane = light range
    );
    
    return proj * view;
}

// ==================== POINT SHADOW MATRIX CALCULATION ====================
// Generates 6 view-projection matrices for omnidirectional (cube map) shadow mapping
inline void computePointShadowMatrices(const PointLight& light, Quark::Mat4 outMatrices[POINT_SHADOW_FACE_COUNT])
{
    // 91 degree FOV perspective - slightly larger than 90° to create overlap and prevent seams
    constexpr float PI = 3.14159265359f;
    constexpr float FOV_OVERLAP = 1.02f;  // 2% overlap to prevent seams at cube face edges
    Quark::Mat4 proj = Quark::Mat4::Perspective(
        (PI / 2.0f) * FOV_OVERLAP,  // ~91.8 degree FOV
        1.0f,                        // Aspect 1:1 (square)
        0.1f,                        // Near plane
        light.range                  // Far plane = light range
    );
    
    // Cube face directions: +X, -X, +Y, -Y, +Z, -Z
    const Quark::Vec3 directions[POINT_SHADOW_FACE_COUNT] = {
        { 1.0f,  0.0f,  0.0f},  // +X
        {-1.0f,  0.0f,  0.0f},  // -X
        { 0.0f,  1.0f,  0.0f},  // +Y
        { 0.0f, -1.0f,  0.0f},  // -Y
        { 0.0f,  0.0f,  1.0f},  // +Z
        { 0.0f,  0.0f, -1.0f}   // -Z
    };
    
    // Up vectors for each face (Y faces need special handling)
    const Quark::Vec3 ups[POINT_SHADOW_FACE_COUNT] = {
        { 0.0f,  1.0f,  0.0f},  // +X: up is +Y
        { 0.0f,  1.0f,  0.0f},  // -X: up is +Y
        { 0.0f,  0.0f, -1.0f},  // +Y: up is -Z
        { 0.0f,  0.0f,  1.0f},  // -Y: up is +Z
        { 0.0f,  1.0f,  0.0f},  // +Z: up is +Y
        { 0.0f,  1.0f,  0.0f}   // -Z: up is +Y
    };
    
    for (UINT32 i = 0; i < POINT_SHADOW_FACE_COUNT; ++i)
    {
        Quark::Mat4 view = Quark::Mat4::LookAt(
            light.position,
            light.position + directions[i],
            ups[i]
        );
        outMatrices[i] = proj * view;
    }
}
