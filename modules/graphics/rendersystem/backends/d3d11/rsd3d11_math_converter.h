#pragma once

// ==================== D3D11 MATH CONVERTER ====================
#include "../../../../headeronly/mathematics.h"
#include "../../framepacket.h"
#include "../../lighting.h"

// ==================== MATRIX CONVERSION ====================

inline Quark::Mat4 convertMatrix(const Quark::Mat4& mat)
{
    return mat.Transposed();
}

inline Quark::Mat4 convertShadowProjectionMatrix(const Quark::Mat4& mat)
{
    // Map [-1, 1] Z to [0, 1] Z
    // Column-major Z-Correction Matrix
    Quark::Mat4 zCorrection = Quark::Mat4::Identity();
    zCorrection.m[10] = 0.5f; 
    zCorrection.m[14] = 0.5f;
    
    return convertMatrix(zCorrection * mat);
}

// ==================== FRAME CONSTANTS CONVERSION ====================
inline FrameConstants convertFrameConstants(const FrameConstants& src)
{
    FrameConstants dst = src;
    dst.view = convertMatrix(src.view);
    dst.projection = convertMatrix(src.projection);
    dst.viewProjection = convertMatrix(src.viewProjection);
    dst.invView = convertMatrix(src.invView);
    dst.invProjection = convertMatrix(src.invProjection);
    return dst;
}

// ==================== INSTANCE DATA CONVERSION ====================
inline PerInstanceData convertInstanceData(const PerInstanceData& src)
{
    PerInstanceData dst = src;
    dst.worldMatrix = convertMatrix(src.worldMatrix);
    dst.worldInvTranspose = convertMatrix(src.worldInvTranspose);
    return dst;
}

// ==================== LIGHT DATA CONVERSION ====================
inline GPULightData convertLightData(const GPULightData& src)
{
    GPULightData dst = src;
    for (UINT32 i = 0; i < DIRECTIONAL_CASCADE_COUNT; ++i)
    {
        dst.cascadeMatrices[i] = convertMatrix(src.cascadeMatrices[i]);
    }
    return dst;
}
