#pragma once

#include <algorithm>
#include <cfloat>
#include <cmath>
#include "../../headeronly/mathematics.h"
#include "camera.h"
#include "lighting.h"

// ==================== CSM CONSTANTS ====================
constexpr float CSM_SPLIT_LAMBDA = 0.5f;   // Lower = more linear = longer close cascades
constexpr float CSM_SHADOW_FAR = 150.0f;
constexpr float SHADOW_CASTER_DISTANCE = 500.0f;

// ==================== CASCADE DATA ====================
struct CascadeData
{
    Quark::Mat4 viewProjMatrix;
    float splitNear;
    float splitFar;
};

// ==================== CSM SYSTEM ====================
struct CSMData
{
    CascadeData cascades[DIRECTIONAL_CASCADE_COUNT];
    Quark::Vec4 splitDistances;
};

// ==================== CSM CALCULATIONS ====================

inline void calculateCascadeSplits(float nearPlane, float farPlane, float lambda, float outSplits[DIRECTIONAL_CASCADE_COUNT + 1])
{
    float clipRange = farPlane - nearPlane;
    float ratio = farPlane / nearPlane;
    
    for (UINT32 i = 0; i <= DIRECTIONAL_CASCADE_COUNT; ++i)
    {
        float p = static_cast<float>(i) / static_cast<float>(DIRECTIONAL_CASCADE_COUNT);
        
        float logSplit = nearPlane * std::pow(ratio, p);
        float linearSplit = nearPlane + clipRange * p;
        
        float dist = lambda * logSplit + (1.0f - lambda) * linearSplit;
        outSplits[i] = dist;
    }
}

inline void computeFrustumCornersWorldSpace(
    const Camera& camera,
    float sliceNear,
    float sliceFar,
    Quark::Vec3 outCorners[8])
{
    // Get camera basis vectors
    Quark::Vec3 forward = camera.forward();
    Quark::Vec3 right = camera.right();
    Quark::Vec3 up = camera.up();
    
    // Calculate frustum dimensions at near and far slices
    float tanHalfFOV = std::tan(camera.fov * 0.5f);
    
    float nearHeight = tanHalfFOV * sliceNear;
    float nearWidth = nearHeight * camera.aspect;
    float farHeight = tanHalfFOV * sliceFar;
    float farWidth = farHeight * camera.aspect;
    
    // Calculate frustum centers
    Quark::Vec3 nearCenter = camera.position + forward * sliceNear;
    Quark::Vec3 farCenter = camera.position + forward * sliceFar;
    
    // Near plane corners
    outCorners[0] = nearCenter - right * nearWidth + up * nearHeight; // top-left
    outCorners[1] = nearCenter + right * nearWidth + up * nearHeight; // top-right
    outCorners[2] = nearCenter + right * nearWidth - up * nearHeight; // bottom-right
    outCorners[3] = nearCenter - right * nearWidth - up * nearHeight; // bottom-left
    
    // Far plane corners
    outCorners[4] = farCenter - right * farWidth + up * farHeight; // top-left
    outCorners[5] = farCenter + right * farWidth + up * farHeight; // top-right
    outCorners[6] = farCenter + right * farWidth - up * farHeight; // bottom-right
    outCorners[7] = farCenter - right * farWidth - up * farHeight; // bottom-left
}

inline Quark::Mat4 computeCascadeMatrix(
    const Quark::Vec3& lightDir,
    const Quark::Vec3 frustumCorners[8],
    UINT32 cascadeIndex,
    float& outWorldUnitsPerTexel)
{
    // ===============================
    // 1. Compute frustum center
    // ===============================
    Quark::Vec3 center = Quark::Vec3::Zero();
    for (int i = 0; i < 8; ++i)
        center += frustumCorners[i];
    center /= 8.0f;

    // ===============================
    // 2. Stable light basis (NO UP FLIP)
    // ===============================
    Quark::Vec3 dir = lightDir.Normalized();
    const Quark::Vec3 WORLD_UP(0, 1, 0);

    Quark::Vec3 right = WORLD_UP.Cross(dir);
    if (right.LengthSq() < 1e-6f)
        right = Quark::Vec3(1, 0, 0);

    right.Normalize();
    Quark::Vec3 up = dir.Cross(right).Normalized();

    // ===============================
    // 3. Compute STABLE cascade radius (prevents shadow shimmering)
    // ===============================
    // Use the diagonal of the frustum slice as a stable radius
    // This doesn't change when camera rotates, only when split distances change
    float maxDist = 0.0f;
    for (int i = 0; i < 8; ++i)
    {
        for (int j = i + 1; j < 8; ++j)
        {
            float dist = (frustumCorners[i] - frustumCorners[j]).Length();
            maxDist = (std::max)(maxDist, dist);
        }
    }
    // Radius is half the maximum diagonal
    float radius = maxDist * 0.5f;
    
    // Quantize radius to texel size to prevent sub-texel changes
    float cascadeResolution = static_cast<float>(DIRECTIONAL_SHADOW_ATLAS_SIZE) * 0.5f;
    float texelSize = (radius * 2.0f) / cascadeResolution;
    radius = std::ceil(radius / texelSize) * texelSize;

    // ===============================
    // 4. Build initial light view
    // ===============================
    // Use large offset to capture shadow casters outside camera frustum
    // (tall buildings, trees, etc. that cast shadows into view)
    Quark::Vec3 lightPos = center - dir * SHADOW_CASTER_DISTANCE;
    Quark::Mat4 lightView = Quark::Mat4::LookAt(lightPos, center, up);

    // ===============================
    // 5. Compute light-space bounds
    // ===============================
    float minX = FLT_MAX, maxX = -FLT_MAX;
    float minY = FLT_MAX, maxY = -FLT_MAX;
    float minZ = FLT_MAX, maxZ = -FLT_MAX;

    for (int i = 0; i < 8; ++i)
    {
        Quark::Vec4 ls = lightView * Quark::Vec4(frustumCorners[i], 1.0f);
        minX = (std::min)(minX, ls.x);
        maxX = (std::max)(maxX, ls.x);
        minY = (std::min)(minY, ls.y);
        maxY = (std::max)(maxY, ls.y);
        minZ = (std::min)(minZ, ls.z);
        maxZ = (std::max)(maxZ, ls.z);
    }

    // ===============================
    // 6. Texel snapping (CRITICAL)
    // ===============================
    // Use STABLE radius for texel size calculation (not dynamic extents)
    // This prevents shadow shimmering when camera rotates
    outWorldUnitsPerTexel = (radius * 2.0f) / cascadeResolution;

    // Snap center IN LIGHT SPACE
    Quark::Vec4 centerLS = lightView * Quark::Vec4(center, 1.0f);
    centerLS.x = std::floor(centerLS.x / outWorldUnitsPerTexel) * outWorldUnitsPerTexel;
    centerLS.y = std::floor(centerLS.y / outWorldUnitsPerTexel) * outWorldUnitsPerTexel;

    Quark::Vec4 snappedCenterWS4 =
        lightView.Inverted() *
        Quark::Vec4(centerLS.x, centerLS.y, centerLS.z, 1.0f);

    Quark::Vec3 snappedCenter(
        snappedCenterWS4.x,
        snappedCenterWS4.y,
        snappedCenterWS4.z
    );

    lightPos = snappedCenter - dir * SHADOW_CASTER_DISTANCE;
    lightView = Quark::Mat4::LookAt(lightPos, snappedCenter, up);

    // ===============================
    // 7. Final ortho bounds (square)
    // ===============================
    float halfSize = std::ceil(radius / outWorldUnitsPerTexel) * outWorldUnitsPerTexel;

    // Z bounds adjusted for large shadow caster distance
    // Near plane starts just in front of light, far extends through entire scene
    float nearZ = 0.1f;
    float farZ = SHADOW_CASTER_DISTANCE + radius * 2.0f;

    // RH system, -Z forward
    Quark::Mat4 lightProj =
        Quark::Mat4::Orthographic(
            -halfSize, halfSize,
            -halfSize, halfSize,
            nearZ,
            farZ
        );

    return lightProj * lightView;
}

inline CSMData computeCSM(const Camera& camera, const Quark::Vec3& lightDir)
{
    CSMData csm = {};
    
    // Calculate cascade splits using practical split scheme
    float effectiveFar = (std::min)(camera.farPlane, CSM_SHADOW_FAR);
    float splits[5];
    calculateCascadeSplits(camera.nearPlane, effectiveFar, CSM_SPLIT_LAMBDA, splits);
    
    csm.splitDistances.x = splits[1];
    csm.splitDistances.y = splits[2];
    csm.splitDistances.z = splits[3];
    csm.splitDistances.w = splits[4];
    
    Quark::Vec3 dir = lightDir.Normalized();
    
    // Compute each cascade
    for (UINT32 i = 0; i < DIRECTIONAL_CASCADE_COUNT; ++i)
    {
        float cascadeNear = splits[i];
        float cascadeFar = splits[i + 1];
        
        // Get frustum corners for this cascade slice
        Quark::Vec3 corners[8];
        computeFrustumCornersWorldSpace(camera, cascadeNear, cascadeFar, corners);
        
        float texelSize = 0.0f;
        csm.cascades[i].viewProjMatrix = computeCascadeMatrix(dir, corners, i, texelSize);
        csm.cascades[i].splitNear = cascadeNear;
        csm.cascades[i].splitFar = cascadeFar;
    }
    
    return csm;
}

inline Quark::Vec2 getCascadeAtlasOffset(UINT32 cascadeIndex)
{
    switch (cascadeIndex)
    {
        case 0: return Quark::Vec2(0.0f, 0.0f);
        case 1: return Quark::Vec2(0.5f, 0.0f);
        case 2: return Quark::Vec2(0.0f, 0.5f);
        case 3: return Quark::Vec2(0.5f, 0.5f);
        default: return Quark::Vec2(0.0f, 0.0f);
    }
}
