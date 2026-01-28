// ==================== rsd3d11_main_ps.h ====================
// DirectX 11 PBR Pixel Shader with Multi-Light Support
#pragma once
#include <string>

// Part 1A: Constants, Structs, Resources
static const char* g_PBRPixelShaderSource_Part1A = R"(
// ==================== CONSTANTS ====================
#ifndef MAX_LIGHTS
#define MAX_LIGHTS 64
#endif

#ifndef PI
#define PI 3.14159265359
#endif

#ifndef DIRECTIONAL_CASCADE_COUNT
#define DIRECTIONAL_CASCADE_COUNT 4
#endif

#ifndef LOCAL_SHADOW_GRID_SIZE
#define LOCAL_SHADOW_GRID_SIZE 8
#endif

#ifndef POINT_SHADOW_FACE_COUNT
#define POINT_SHADOW_FACE_COUNT 6
#endif

// Light types
#ifndef LIGHT_TYPE_NONE
#define LIGHT_TYPE_NONE 0
#endif
#ifndef LIGHT_TYPE_DIRECTIONAL
#define LIGHT_TYPE_DIRECTIONAL 1
#endif
#ifndef LIGHT_TYPE_POINT
#define LIGHT_TYPE_POINT 2
#endif
#ifndef LIGHT_TYPE_SPOT
#define LIGHT_TYPE_SPOT 3
#endif

// Light flags (must match C++ LightFlags enum)
#ifndef LIGHT_FLAG_ENABLED
#define LIGHT_FLAG_ENABLED      0x01
#endif
#ifndef LIGHT_FLAG_CAST_SHADOWS
#define LIGHT_FLAG_CAST_SHADOWS 0x02
#endif

// RenderObject flags (must match C++ RenderObjectFlags enum)
#define OBJFLAG_STATIC         0x01
#define OBJFLAG_DYNAMIC        0x02
#define OBJFLAG_VISIBLE        0x04
#define OBJFLAG_CAST_SHADOW    0x08
#define OBJFLAG_RECEIVE_SHADOW 0x10
#define OBJFLAG_FRUSTUM_CULL   0x20

// Material flags
#define MATFLAG_ALBEDO_MAP    0x01
#define MATFLAG_NORMAL_MAP    0x02
#define MATFLAG_METALLIC_MAP  0x04
#define MATFLAG_ROUGHNESS_MAP 0x08
#define MATFLAG_AO_MAP        0x10
#define MATFLAG_EMISSIVE_MAP  0x20
#define MATFLAG_ALPHA_BLEND   0x40

// ==================== GPU LIGHT DATA ====================
struct GPULight
{
    float3 position;
    uint type;
    
    float3 direction;
    float range;
    
    float3 color;
    float intensity;
    
    float2 spotAngles;
    uint shadowQuality;  // 0=None, 1=Hard(1x1), 2=Medium(3x3), 3=Soft(5x5)
    uint flags;
    
    float3 attenuation;
    int shadowMapIndex;
    
    // CSM data
    float4 cascadeSplits;
    matrix cascadeMatrices[DIRECTIONAL_CASCADE_COUNT];
    
    // Spot light shadow matrix
    matrix spotShadowMatrix;
    
    // Point light cube face shadow matrices
    matrix pointShadowMatrices[POINT_SHADOW_FACE_COUNT];
};

// ==================== FRAME CONSTANTS ====================
cbuffer FrameConstants : register(b0)
{
    matrix g_View;
    matrix g_Projection;
    matrix g_ViewProjection;
    matrix g_InvView;
    matrix g_InvProjection;
    
    float3 g_CameraPosition;
    float g_Time;
    
    float4 g_AmbientLight;
    
    float g_DeltaTime;
    uint g_ActiveLightCount;
    uint g_ShadowAtlasSize;
    uint g_LocalShadowAtlasSize;  // Spot/Point shadow atlas size
};

// ==================== MATERIAL CONSTANTS ====================
cbuffer MaterialConstants : register(b1)
{
    float4 g_Albedo;
    
    float g_Metallic;
    float g_Roughness;
    float g_AO;
    float g_AlphaCutoff;
    
    uint g_CullMode;
    
    // Advanced PBR
    float g_Anisotropy;
    float g_ClearCoat;
    float g_ClearCoatRoughness;
    
    float4 g_Emissive;
    
    float g_EmissiveStrength;
    uint g_AlbedoMap;
    uint g_NormalMap;
    uint g_MetallicMap;
    
    uint g_RoughnessMap;
    uint g_AOMap;
    uint g_EmissiveMap;
    uint g_Flags;
};

// ==================== RESOURCES ====================
StructuredBuffer<GPULight> g_Lights : register(t6);

Texture2D g_AlbedoTexture : register(t0);
Texture2D g_NormalTexture : register(t1);
Texture2D g_MetallicTexture : register(t2);
Texture2D g_RoughnessTexture : register(t3);
Texture2D g_AOTexture : register(t4);
Texture2D g_EmissiveTexture : register(t5);

// Shadow Maps
Texture2D g_ShadowAtlas : register(t10);       // Directional CSM
Texture2D g_LocalShadowAtlas : register(t11);  // Spot/Point shadows

SamplerState g_Sampler : register(s0);
SamplerComparisonState g_ShadowSampler : register(s1);

// ==================== PIXEL INPUT ====================
struct PS_INPUT
{
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float2 texCoord : TEXCOORD2;
    float3 tangent : TEXCOORD3;
    float3 bitangent : TEXCOORD4;
    float instanceFlags : TEXCOORD5;  // RenderObjectFlags from CPU
};
)";

// Part 1B: PBR Functions, CSM Functions
static const char* g_PBRPixelShaderSource_Part1B = R"(
// ==================== PBR FUNCTIONS ====================
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return num / max(denom, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

// Height-correlated Smith geometry (more accurate)
float GeometrySmithCorrelated(float NdotV, float NdotL, float roughness)
{
    float a2 = roughness * roughness * roughness * roughness;
    float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - a2) + a2);
    float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - a2) + a2);
    return 0.5 / max(GGXV + GGXL, 0.0001);
}

// ==================== ANISOTROPIC BRDF ====================
float DistributionGGXAnisotropic(float NdotH, float3 H, float3 T, float3 B, float at, float ab)
{
    float TdotH = dot(T, H);
    float BdotH = dot(B, H);
    float a2 = at * ab;
    float3 v = float3(ab * TdotH, at * BdotH, a2 * NdotH);
    float v2 = dot(v, v);
    float w2 = a2 / v2;
    return a2 * w2 * w2 * (1.0 / PI);
}

float VisibilitySmithGGXCorrelatedAnisotropic(float NdotV, float NdotL, float3 V, float3 L, float3 T, float3 B, float at, float ab)
{
    float TdotV = dot(T, V);
    float BdotV = dot(B, V);
    float TdotL = dot(T, L);
    float BdotL = dot(B, L);
    
    float lambdaV = NdotL * length(float3(at * TdotV, ab * BdotV, NdotV));
    float lambdaL = NdotV * length(float3(at * TdotL, ab * BdotL, NdotL));
    
    return 0.5 / max(lambdaV + lambdaL, 0.0001);
}

// ==================== CLEAR COAT BRDF ====================
// Clear coat uses a separate specular lobe (usually with low roughness)
float DistributionGGXClearCoat(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;
    float num = a2 - 1.0;
    float denom = (NdotH2 * num + 1.0);
    return a2 / (PI * denom * denom);
}

float GeometrySmithClearCoat(float NdotV, float NdotL, float roughness)
{
    // Kelemen approximation for clear coat
    return 0.25 / (max(NdotV + NdotL, 0.0001));
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

// Fresnel with roughness - essential for IBL
float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

// Multi-scattering energy compensation (Kulla-Conty approximation)
float3 MultiscatterCompensation(float3 F, float NdotV, float NdotL, float roughness)
{
    // Approximate energy loss from single-scatter BRDF
    float Ess = 0.04 + (1.0 - 0.04) * pow(1.0 - NdotV, 5.0);
    float Ems = 1.0 - Ess;
    
    // Compensation factor
    float3 Favg = F * 0.087 + 0.0425;  // Average Fresnel approximation
    float3 Fms = Favg * Favg * Ems / (1.0 - Favg * Ems);
    
    return 1.0 + Fms;
}

float CalculateAttenuation(float dist, float range, float constAtten, float linAtten, float quadAtten)
{
    float attenuation = 1.0 / (constAtten + linAtten * dist + quadAtten * dist * dist);
    float falloff = saturate(1.0 - (dist / range));
    return attenuation * falloff * falloff;
}

// Physically correct inverse-square attenuation with smooth falloff
float CalculatePhysicalAttenuation(float distance, float range)
{
    float distanceSquared = distance * distance;
    float rangeSquared = range * range;
    
    // Inverse square with smooth falloff at range boundary
    float factor = distanceSquared / rangeSquared;
    float smoothFalloff = saturate(1.0 - factor * factor);
    smoothFalloff *= smoothFalloff;
    
    return smoothFalloff / max(distanceSquared, 0.01);
}

// ==================== IBL (IMAGE-BASED LIGHTING) ====================
// Approximate environment reflection without cubemap (sky gradient)
float3 ApproximateEnvironment(float3 R, float roughness)
{
    // Normalize direction for safety
    float3 dir = normalize(R);
    
    // Smooth gradient based on Y direction (up/down)
    float upFactor = dir.y * 0.5 + 0.5;  // 0 = down, 0.5 = horizon, 1 = up
    
    // Sky colors
    float3 zenithColor = float3(0.2, 0.4, 0.8);   // Deep blue sky
    float3 horizonColor = float3(0.7, 0.75, 0.85); // Light horizon
    float3 groundColor = float3(0.25, 0.22, 0.18); // Dark ground
    
    // Smooth blending between ground → horizon → sky (no if/else!)
    // Use smoothstep for extra smooth transition
    float skyBlend = smoothstep(0.0, 1.0, upFactor);
    float groundBlend = smoothstep(0.5, 0.0, upFactor);
    
    // Interpolate: ground ↔ horizon ↔ sky
    float3 envColor = lerp(horizonColor, zenithColor, skyBlend * skyBlend);
    envColor = lerp(envColor, groundColor, groundBlend);
    
    // Rougher surfaces see more blurred/averaged environment
    float3 avgEnvColor = float3(0.5, 0.52, 0.55);  // Average environment color
    envColor = lerp(envColor, avgEnvColor, roughness * 0.6);
    
    return envColor;
}

// BRDF integration lookup approximation (replaces LUT texture)
float2 IntegrateBRDFApprox(float NdotV, float roughness)
{
    // Analytical approximation from "Real Shading in Unreal Engine 4"
    const float4 c0 = float4(-1.0, -0.0275, -0.572, 0.022);
    const float4 c1 = float4(1.0, 0.0425, 1.04, -0.04);
    float4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NdotV)) * r.x + r.y;
    return float2(-1.04, 1.04) * a004 + r.zw;
}

// Calculate IBL contribution (ambient specular + diffuse)
float3 CalculateIBL(float3 N, float3 V, float3 R, float3 albedo, float3 F0, float roughness, float metallic, float ao)
{
    float NdotV = max(dot(N, V), 0.0);
    
    // Fresnel for IBL (considers roughness)
    float3 F = FresnelSchlickRoughness(NdotV, F0, roughness);
    
    // Specular/Diffuse split
    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - metallic);
    
    // Diffuse IBL (irradiance - use hemisphere average)
    float3 irradiance = ApproximateEnvironment(N, 1.0) * 0.5;  // Heavily blurred for diffuse
    float3 diffuseIBL = kD * albedo * irradiance;
    
    // Specular IBL (pre-filtered environment)
    float3 prefilteredColor = ApproximateEnvironment(R, roughness);
    float2 brdfLUT = IntegrateBRDFApprox(NdotV, roughness);
    float3 specularIBL = prefilteredColor * (F * brdfLUT.x + brdfLUT.y);
    
    // Combine with AO
    float3 ambient = (diffuseIBL + specularIBL) * ao;
    
    return ambient;
}

// ==================== CSM SHADOW CALCULATION ====================
// Select cascade based on view-space depth
uint SelectCascade(float viewSpaceZ, float4 cascadeSplits)
{
    float depth = abs(viewSpaceZ);
    if (depth < cascadeSplits.x) return 0;
    if (depth < cascadeSplits.y) return 1;
    if (depth < cascadeSplits.z) return 2;
    return 3;
}

// Get atlas offset for cascade (2x2 grid)
float2 GetCascadeAtlasOffset(uint cascadeIndex)
{
    switch (cascadeIndex)
    {
        case 0: return float2(0.0, 0.0);
        case 1: return float2(0.5, 0.0);
        case 2: return float2(0.0, 0.5);
        case 3: return float2(0.5, 0.5);
        default: return float2(0.0, 0.0);
    }
}

// Sample shadow for a specific cascade
float SampleCascadeShadow(float3 worldPos, uint cascadeIndex, GPULight light, float softness, float3 N, float3 L)
{
    // Normal offset bias - push sample point along normal to reduce self-shadowing
    // Larger cascades need larger offset
    float normalOffsetScale = 0.02 * (cascadeIndex + 1);
    float3 offsetPos = worldPos + N * normalOffsetScale;
    
    // Transform to light space
    float4 shadowPos = mul(float4(offsetPos, 1.0), light.cascadeMatrices[cascadeIndex]);
    
    // Perspective divide
    float3 projCoords = shadowPos.xyz / shadowPos.w;
    
    // Transform from [-1, 1] to [0, 1]
    projCoords.x = projCoords.x * 0.5 + 0.5;
    projCoords.y = -projCoords.y * 0.5 + 0.5;
    
    // Check if out of frustum
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 1.0;
    
    // Apply atlas offset and scale for 2x2 grid
    float2 atlasOffset = GetCascadeAtlasOffset(cascadeIndex);
    float2 atlasUV = projCoords.xy * 0.5 + atlasOffset;
    
    // Slope-based depth bias to prevent shadow acne
    float NdotL = saturate(dot(N, L));
    float slopeBias = 0.0005 * (1.0 - NdotL);
    float constantBias = 0.0001 * (cascadeIndex + 1); // Larger bias for further cascades
    float currentDepth = projCoords.z - (slopeBias + constantBias);
    
    // PCF filtering based on shadow quality
    float shadow = 0.0;
    float2 texelSize = 1.0 / (float)g_ShadowAtlasSize;
    uint quality = light.shadowQuality;
    
    // Quality: 0=None, 1=Hard(1x1), 2=Medium(3x3), 3=Soft(5x5)
    if (quality <= 1)
    {
        // Hard shadows - 1x1 (single sample)
        shadow = g_ShadowAtlas.SampleCmpLevelZero(g_ShadowSampler, atlasUV, currentDepth);
    }
    else if (quality == 2)
    {
        // Medium shadows - 3x3 PCF
        for (int x = -1; x <= 1; ++x)
        {
            for (int y = -1; y <= 1; ++y)
            {
                float2 offset = float2(x, y) * texelSize * softness;
                shadow += g_ShadowAtlas.SampleCmpLevelZero(g_ShadowSampler, atlasUV + offset, currentDepth);
            }
        }
        shadow /= 9.0;
    }
    else
    {
        // Soft shadows - 5x5 PCF
        for (int x = -2; x <= 2; ++x)
        {
            for (int y = -2; y <= 2; ++y)
            {
                float2 offset = float2(x, y) * texelSize * softness;
                shadow += g_ShadowAtlas.SampleCmpLevelZero(g_ShadowSampler, atlasUV + offset, currentDepth);
            }
        }
        shadow /= 25.0;
    }
    
    return shadow;
}

// CSM shadow calculation with cascade blending
float CalculateCSMShadow(float3 worldPos, float viewSpaceZ, float3 N, float3 L, GPULight light)
{
    float depth = abs(viewSpaceZ);
    
    // Get cascade splits
    float4 splits = light.cascadeSplits;
    float cascadeSplits[5] = { 0.0, splits.x, splits.y, splits.z, splits.w };
    
    // Select primary cascade
    uint cascadeIndex = SelectCascade(viewSpaceZ, splits);
    
    // Calculate softness (default 1.0 for PCF spread)
    float softness = 1.0;
    
    // Sample primary cascade
    float shadow = SampleCascadeShadow(worldPos, cascadeIndex, light, softness, N, L);
    
    // Cascade blending: blend with next cascade near the boundary
    // This eliminates the visible seam between cascades
    if (cascadeIndex < 3)
    {
        float nextCascadeSplit = cascadeSplits[cascadeIndex + 1];
        float currentCascadeSplit = cascadeSplits[cascadeIndex];
        float cascadeRange = nextCascadeSplit - currentCascadeSplit;
        
        // Blend zone is 10% of cascade range at the far end
        float blendZoneStart = nextCascadeSplit - cascadeRange * 0.1;
        
        if (depth > blendZoneStart)
        {
            // Calculate blend factor (0 at blend start, 1 at cascade boundary)
            float blendFactor = saturate((depth - blendZoneStart) / (nextCascadeSplit - blendZoneStart));
            
            // Sample next cascade and blend
            float nextShadow = SampleCascadeShadow(worldPos, cascadeIndex + 1, light, softness, N, L);
            shadow = lerp(shadow, nextShadow, blendFactor);
        }
    }
    
    return shadow;
}

// Legacy single shadow (for point/spot lights)
float CalculateShadow(float4 shadowPos, float3 N, float3 L, float softness, float userBias)
{
    // Perspective divide
    float3 projCoords = shadowPos.xyz / shadowPos.w;
    
    // Transform from [-1, 1] to [0, 1]
    projCoords.x = projCoords.x * 0.5 + 0.5;
    projCoords.y = -projCoords.y * 0.5 + 0.5;
    
    // Check if out of frustum
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 1.0;
        
    // Bias to prevent acne (based on slope)
    // Use user-provided bias as the maximum bias
    float bias = max(userBias * (1.0 - dot(N, L)), userBias * 0.1);  
    // Note: We already use SlopeScaledDepthBias in rasterizer, so we can use smaller bias here or 0.
    // Let's use the comparison sampler's hardware logic mainly.
    // But modifying the comparison depth slightly helps.
    
    float currentDepth = projCoords.z - bias;
    
    // PCF (Percentage-Closer Filtering) 3x3
    // Balanced softness. 9 samples.
    
    float shadow = 0.0;
    float2 texelSize = 1.0 / (float)g_ShadowAtlasSize; 
    
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float2 offset = float2(x, y) * texelSize * softness;
            shadow += g_ShadowAtlas.SampleCmpLevelZero(g_ShadowSampler, projCoords.xy + offset, currentDepth);
        }
    }
    
    return shadow / 9.0;
}
)";

// Part 2A: Spot/Point Shadow Functions
static const char* g_PBRPixelShaderSource_Part2A = R"(
// ==================== SPOT LIGHT SHADOW CALCULATION ====================
float CalculateSpotShadow(float3 worldPos, GPULight light, float3 N, float3 L)
{
    // Transform to light space
    float4 shadowPos = mul(float4(worldPos, 1.0), light.spotShadowMatrix);
    
    // Perspective divide
    float3 projCoords = shadowPos.xyz / shadowPos.w;
    
    // Transform from [-1, 1] to [0, 1]
    projCoords.x = projCoords.x * 0.5 + 0.5;
    projCoords.y = -projCoords.y * 0.5 + 0.5;
    
    // Check if out of frustum
    if (projCoords.z > 1.0 || projCoords.z < 0.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 || 
        projCoords.y < 0.0 || projCoords.y > 1.0)
        return 1.0;
    
    // Calculate atlas UV (grid, slot index in shadowMapIndex)
    float gridSize = float(LOCAL_SHADOW_GRID_SIZE);
    float slotSize = 1.0 / gridSize;
    float col = float(light.shadowMapIndex % LOCAL_SHADOW_GRID_SIZE);
    float row = float(light.shadowMapIndex / LOCAL_SHADOW_GRID_SIZE);
    float2 atlasOffset = float2(col, row) * slotSize;
    float2 atlasUV = projCoords.xy * slotSize + atlasOffset;
    
    // Bias to prevent shadow acne
    float NdotL = saturate(dot(N, L));
    float bias = max(0.001 * (1.0 - NdotL), 0.0005);
    float currentDepth = projCoords.z - bias;
    
    // PCF filtering based on shadow quality
    float shadow = 0.0;
    float2 texelSize = 1.0 / (float)g_LocalShadowAtlasSize;
    uint quality = light.shadowQuality;
    
    if (quality <= 1)
    {
        // Hard shadows - single sample
        shadow = g_LocalShadowAtlas.SampleCmpLevelZero(g_ShadowSampler, atlasUV, currentDepth);
    }
    else if (quality == 2)
    {
        // Medium shadows - 3x3 PCF
        for (int x = -1; x <= 1; ++x)
        {
            for (int y = -1; y <= 1; ++y)
            {
                float2 offset = float2(x, y) * texelSize;
                shadow += g_LocalShadowAtlas.SampleCmpLevelZero(g_ShadowSampler, atlasUV + offset, currentDepth);
            }
        }
        shadow /= 9.0;
    }
    else
    {
        // Soft shadows - 5x5 PCF
        for (int x = -2; x <= 2; ++x)
        {
            for (int y = -2; y <= 2; ++y)
            {
                float2 offset = float2(x, y) * texelSize;
                shadow += g_LocalShadowAtlas.SampleCmpLevelZero(g_ShadowSampler, atlasUV + offset, currentDepth);
            }
        }
        shadow /= 25.0;
    }
    
    return shadow;
}

// ==================== POINT LIGHT SHADOW CALCULATION (CUBE MAP) ====================
// Select which cube face the fragment falls on based on light-to-fragment direction
uint SelectCubeFace(float3 lightToFrag)
{
    float3 absDir = abs(lightToFrag);
    
    if (absDir.x >= absDir.y && absDir.x >= absDir.z)
        return (lightToFrag.x > 0) ? 0 : 1;  // +X or -X
    else if (absDir.y >= absDir.x && absDir.y >= absDir.z)
        return (lightToFrag.y > 0) ? 2 : 3;  // +Y or -Y
    else
        return (lightToFrag.z > 0) ? 4 : 5;  // +Z or -Z
}

float CalculatePointShadow(float3 worldPos, GPULight light, float3 N, float3 L)
{
    // Get direction from light to fragment
    float3 lightToFrag = worldPos - light.position;
    
    // Select which cube face to sample
    uint faceIndex = SelectCubeFace(lightToFrag);
    
    // Transform to this face's light space
    float4 shadowPos = mul(float4(worldPos, 1.0), light.pointShadowMatrices[faceIndex]);
    
    // Perspective divide
    float3 projCoords = shadowPos.xyz / shadowPos.w;
    
    // Transform from [-1, 1] to [0, 1]
    projCoords.x = projCoords.x * 0.5 + 0.5;
    projCoords.y = -projCoords.y * 0.5 + 0.5;
    
    // Check if out of frustum
    if (projCoords.z > 1.0 || projCoords.z < 0.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 || 
        projCoords.y < 0.0 || projCoords.y > 1.0)
        return 1.0;
    
    // Calculate atlas slot: base slot + face index
    uint slotIndex = light.shadowMapIndex + faceIndex;
    
    // Calculate atlas UV (8x8 grid) with edge padding to prevent bleeding
    float gridSize = float(LOCAL_SHADOW_GRID_SIZE);
    float slotSize = 1.0 / gridSize;
    float col = float(slotIndex % LOCAL_SHADOW_GRID_SIZE);
    float row = float(slotIndex / LOCAL_SHADOW_GRID_SIZE);
    float2 atlasOffset = float2(col, row) * slotSize;
    
    // Add edge padding: shrink UV range slightly to avoid sampling adjacent slots
    float edgePadding = 0.5 / (float)g_LocalShadowAtlasSize;  // Half texel padding
    float2 clampedUV = clamp(projCoords.xy, edgePadding, 1.0 - edgePadding);
    float2 atlasUV = clampedUV * slotSize + atlasOffset;
    
    // Bias to prevent shadow acne
    float NdotL = saturate(dot(N, L));
    float bias = max(0.002 * (1.0 - NdotL), 0.001);
    float currentDepth = projCoords.z - bias;
    
    // PCF filtering based on shadow quality
    float shadow = 0.0;
    float2 texelSize = 1.0 / (float)g_LocalShadowAtlasSize;
    uint quality = light.shadowQuality;
    
    if (quality <= 1)
    {
        // Hard shadows - single sample
        shadow = g_LocalShadowAtlas.SampleCmpLevelZero(g_ShadowSampler, atlasUV, currentDepth);
    }
    else if (quality == 2)
    {
        // Medium shadows - 3x3 PCF
        for (int x = -1; x <= 1; ++x)
        {
            for (int y = -1; y <= 1; ++y)
            {
                float2 offset = float2(x, y) * texelSize;
                shadow += g_LocalShadowAtlas.SampleCmpLevelZero(g_ShadowSampler, atlasUV + offset, currentDepth);
            }
        }
        shadow /= 9.0;
    }
    else
    {
        // Soft shadows - 5x5 PCF
        for (int x = -2; x <= 2; ++x)
        {
            for (int y = -2; y <= 2; ++y)
            {
                float2 offset = float2(x, y) * texelSize;
                shadow += g_LocalShadowAtlas.SampleCmpLevelZero(g_ShadowSampler, atlasUV + offset, currentDepth);
            }
        }
        shadow /= 25.0;
    }
    
    return shadow;
}
)";

// Part 2B: Main Pixel Shader
static const char* g_PBRPixelShaderSource_Part2B = R"(
// ==================== PIXEL SHADER MAIN ======================================
float4 main(PS_INPUT input) : SV_TARGET
{
    // Material properties
    float3 albedo = g_Albedo.rgb;
    float alpha = g_Albedo.a;
    float metallic = g_Metallic;
    float roughness = g_Roughness;
    float ao = g_AO;
    
    // Sample textures
    if (g_Flags & MATFLAG_ALBEDO_MAP)
    {
        float4 albedoSample = g_AlbedoTexture.Sample(g_Sampler, input.texCoord);
        albedo = albedoSample.rgb;
        alpha *= albedoSample.a;
    }
    
    if (g_Flags & MATFLAG_METALLIC_MAP)
        metallic = g_MetallicTexture.Sample(g_Sampler, input.texCoord).r;
    
    if (g_Flags & MATFLAG_ROUGHNESS_MAP)
        roughness = g_RoughnessTexture.Sample(g_Sampler, input.texCoord).r;
    
    if (g_Flags & MATFLAG_AO_MAP)
        ao = g_AOTexture.Sample(g_Sampler, input.texCoord).r;
    
    // Alpha cutout
    if (g_Flags & MATFLAG_ALPHA_BLEND)
        clip(alpha - g_AlphaCutoff);
    
    // Normal mapping
    float3 N = normalize(input.normal);
    if (g_Flags & MATFLAG_NORMAL_MAP)
    {
        float3 T = normalize(input.tangent);
        float3 B = normalize(input.bitangent);
        float3x3 TBN = float3x3(T, B, N);
        
        float3 normalMap = g_NormalTexture.Sample(g_Sampler, input.texCoord).rgb;
        normalMap = normalMap * 2.0 - 1.0;
        N = normalize(mul(normalMap, TBN));
    }
    
    // View direction
    float3 V = normalize(g_CameraPosition - input.worldPos);
    
    // F0 (base reflectivity)
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedo, metallic);
    
    // ==================== LIGHTING LOOP ====================
    float3 Lo = float3(0.0, 0.0, 0.0);
    
    uint lightCount = min(g_ActiveLightCount, MAX_LIGHTS);
    for (uint i = 0; i < lightCount; i++)
    {
        GPULight light = g_Lights[i];
        
        // Skip disabled lights
        if (!(light.flags & LIGHT_FLAG_ENABLED))
            continue;
        
        float3 L;
        float3 radiance;
        float attenuation = 1.0;
        
        // Calculate light direction and radiance based on type
        if (light.type == LIGHT_TYPE_DIRECTIONAL)
        {
            L = normalize(-light.direction);
            radiance = light.color * light.intensity;
            
            // CSM Shadows for Directional Light
            uint objFlags = (uint)input.instanceFlags;
            
            // Should verify object and light flags for casting shadows
            if ((light.flags & LIGHT_FLAG_CAST_SHADOWS) && (objFlags & OBJFLAG_RECEIVE_SHADOW))
            {
                // Calculate view-space Z for cascade selection
                float4 viewPos = mul(float4(input.worldPos, 1.0), g_View);
                float viewSpaceZ = viewPos.z;
                
                float shadow = CalculateCSMShadow(input.worldPos, viewSpaceZ, N, L, light);
                radiance *= shadow;
            }
        }
        else if (light.type == LIGHT_TYPE_POINT)
        {
            float3 toLight = light.position - input.worldPos;
            float distance = length(toLight);
            L = toLight / distance;
            
            attenuation = CalculateAttenuation(distance, light.range, 
                light.attenuation.x, light.attenuation.y, light.attenuation.z);
            radiance = light.color * light.intensity * attenuation;
            
            // Point light cube map shadows
            uint objFlags = (uint)input.instanceFlags;
            if ((light.flags & LIGHT_FLAG_CAST_SHADOWS) && 
                (objFlags & OBJFLAG_RECEIVE_SHADOW) && 
                light.shadowMapIndex >= 0)
            {
                float shadow = CalculatePointShadow(input.worldPos, light, N, L);
                radiance *= shadow;
            }
        }
        else // LIGHT_TYPE_SPOT
        {
            float3 toLight = light.position - input.worldPos;
            float distance = length(toLight);
            L = toLight / distance;
            
            float theta = dot(L, normalize(-light.direction));
            float epsilon = light.spotAngles.x - light.spotAngles.y;
            float spotIntensity = saturate((theta - light.spotAngles.y) / epsilon);
            
            attenuation = CalculateAttenuation(distance, light.range,
                light.attenuation.x, light.attenuation.y, light.attenuation.z);
            radiance = light.color * light.intensity * attenuation * spotIntensity;
            
            // Spot light shadows
            uint objFlags = (uint)input.instanceFlags;
            if ((light.flags & LIGHT_FLAG_CAST_SHADOWS) && 
                (objFlags & OBJFLAG_RECEIVE_SHADOW) && 
                light.shadowMapIndex >= 0)
            {
                float shadow = CalculateSpotShadow(input.worldPos, light, N, L);
                radiance *= shadow;
            }
        }
        
        // Calculate BRDF
        float3 H = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0);
        
        if (NdotL > 0.0)
        {
            float NDF = 0.0;
            float G = 0.0;
            float3 F = float3(0.0, 0.0, 0.0);
            
            // Anisotropy parameters
            float3 T = normalize(input.tangent);
            float3 B = normalize(input.bitangent);
            
            if (g_Anisotropy > 0.0)
            {
                // Anisotropic GGX
                float at = max(roughness * (1.0 + g_Anisotropy), 0.001);
                float ab = max(roughness * (1.0 - g_Anisotropy), 0.001);
                
                NDF = DistributionGGXAnisotropic(dot(N, H), H, T, B, at, ab);
                G = VisibilitySmithGGXCorrelatedAnisotropic(dot(N, V), NdotL, V, L, T, B, at, ab);
                F = FresnelSchlick(max(dot(H, V), 0.0), F0);
            }
            else
            {
                // Standard Isotropic GGX (Multi-scattering compensated)
                NDF = DistributionGGX(N, H, roughness);
                G = GeometrySmithCorrelated(dot(N, V), NdotL, roughness);
                F = FresnelSchlickRoughness(max(dot(H, V), 0.0), F0, roughness); 
            }

            float3 kS = F;
            float3 kD = (1.0 - kS) * (1.0 - metallic);
            
            // Specular lobe
            float3 numerator = NDF * G * F;
            float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001; // GeometrySmithCorrelated already includes 0.5/denom, so G * F is almost radiance
            
            // Note: GeometrySmithCorrelated returns 0.5 / (GGXV + GGXL) which is V (Visibility)
            // Specular = D * V * F (where V includes the 1/(4*N.V*N.L) term implicit or explicit)
            // My DistributionGGX returns D.
            // My GeometrySmithCorrelated returns V term directly (Visibility).
            // So Specular = D * G * F.
            // Let's adjust based on function definitions.
            
            // Adjusting based on standard implementations: 
            // Standard: (D * G * F) / (4 * N.V * N.L)
            // My previous GeometrySmithCorrelated: returns 0.5 / (...) which IS the Vis term.
            // So simply: specular = NDF * G * F
            
            // However, keeping consistency with existing code if possible.
            // The existing GeometrySmith was standard G, requiring / 4*N.V*N.L
            // The new GeometrySmithCorrelated is Visibility term.
            
            float3 specular;
            if (g_Anisotropy > 0.0)
            {
               // VisibilitySmithGGXCorrelatedAnisotropic returns Visibility term
               specular = NDF * G * F; 
            }
            else
            {
               // Standard path uses correct Visibility term now (GeometrySmithCorrelated)
               specular = NDF * G * F; 
            }
            
            // Clear Coat Layer
            if (g_ClearCoat > 0.0)
            {
                float NdotH = max(dot(N, H), 0.0);
                float Dcc = DistributionGGXClearCoat(NdotH, g_ClearCoatRoughness);
                float Gcc = GeometrySmithClearCoat(dot(N, V), NdotL, g_ClearCoatRoughness);
                float3 Fcc = FresnelSchlick(max(dot(H, V), 0.0), float3(0.04, 0.04, 0.04)) * g_ClearCoat;
                
                float3 clearCoatSpecular = Dcc * Gcc * Fcc;
                
                // Mix layers (energy conservation)
                // Base layer is attenuated by clear coat fresnel
                float3 attenuation = 1.0 - Fcc;
                specular = specular * attenuation + clearCoatSpecular;
                kD *= attenuation;
            }
            
            Lo += (kD * albedo / PI + specular) * radiance * NdotL;
        }
    }
    
    // ==================== IBL AMBIENT ====================
    // Calculate reflection vector
    float3 R = reflect(-V, N);
    
    // IBL-based ambient lighting (replaces flat ambient)
    float3 ambient = CalculateIBL(N, V, R, albedo, F0, roughness, metallic, ao);
    
    // Add base ambient light contribution (user-defined ambient intensity)
    ambient += g_AmbientLight.rgb * albedo * ao * 0.1;  // Reduced flat ambient, mostly IBL
    
    // Emissive
    float3 emissive = g_Emissive.rgb * g_EmissiveStrength;
    if (g_Flags & MATFLAG_EMISSIVE_MAP)
        emissive += g_EmissiveTexture.Sample(g_Sampler, input.texCoord).rgb * g_EmissiveStrength;
    
    // Final color
    float3 color = ambient + Lo + emissive;
    
    // ==================== IMPROVED ACES FILMIC TONE MAPPING ====================
    // Using fitted ACES approximation for better highlight compression
    float3 x = color;
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    color = saturate((x * (a * x + b)) / (x * (c * x + d) + e));
    
    // Gamma correction
    color = pow(color, 1.0 / 2.2);
    
    return float4(color, alpha);
}
)";

// Combined shader source
static std::string g_PBRPixelShaderSource = 
    std::string(g_PBRPixelShaderSource_Part1A) + 
    std::string(g_PBRPixelShaderSource_Part1B) + 
    std::string(g_PBRPixelShaderSource_Part2A) + 
    std::string(g_PBRPixelShaderSource_Part2B);
