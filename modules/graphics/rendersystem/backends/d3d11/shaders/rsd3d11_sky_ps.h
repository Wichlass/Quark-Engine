// ==================== rsd3d11_sky_ps.h ====================
// Sky Pixel Shader - Volumetric Clouds with 3D Noise and Ray Marching
// Unreal Engine / Source 2 Quality Implementation
#pragma once
#include <string>

// ==================== Part 1: Constants & Flags ====================
static const char* g_SkyPixelShaderSource_Part1 = R"(
// ==================== SKY CONSTANTS ====================
cbuffer SkyConstants : register(b2)
{
    // Row 0: Flags & Quality
    uint g_SkyFlags;                        // 4 bytes
    uint g_SkyQuality;                      // 4 bytes
    uint g_VolumetricQuality;               // 4 bytes
    float g_Time;                           // 4 bytes = 16
    
    // Row 1-4: Sun
    float3 g_SunDirection;                  // 12 bytes
    float g_SunIntensity;                   // 4 bytes = 16
    float3 g_SunColorZenith;                // 12 bytes
    float g_SunDiskSize;                    // 4 bytes = 16
    float3 g_SunColorHorizon;               // 12 bytes
    float g_SunDiskSoftness;                // 4 bytes = 16
    float3 g_SunAverageColor;               // 12 bytes
    float g_SunBloomStrength;               // 4 bytes = 16
    
    // Row 5-7: Atmosphere
    float g_RayleighStrength;               // 4 bytes
    float g_MieStrength;                    // 4 bytes
    float g_MieAnisotropy;                  // 4 bytes
    float g_AtmosphereHeight;               // 4 bytes = 16
    float3 g_HorizonColor;                  // 12 bytes
    float g_DensityFalloff;                 // 4 bytes = 16
    float3 g_ZenithColor;                   // 12 bytes
    float g_AtmospherePlanetRadius;         // 4 bytes = 16
    
    // Row 8-10: Cloud 2D
    float g_CloudCoverage;                  // 4 bytes
    float g_CloudDensity;                   // 4 bytes
    float g_CloudHeight;                    // 4 bytes
    float g_CloudThickness;                 // 4 bytes = 16
    float g_CloudSpeed;                     // 4 bytes
    float g_CloudSunLightInfluence;         // 4 bytes
    float2 g_CloudDirection;                // 8 bytes = 16
    float3 g_CloudColor;                    // 12 bytes
    float g_CloudLightAbsorption;           // 4 bytes = 16
    
    // Row 11-14: Volumetric Cloud Shape
    float g_CloudLayerBottom;               // 4 bytes
    float g_CloudLayerTop;                  // 4 bytes
    float g_CloudScale;                     // 4 bytes
    float g_CloudDetailScale;               // 4 bytes = 16
    float g_CloudErosion;                   // 4 bytes
    float g_CloudCurliness;                 // 4 bytes
    float g_CloudWeatherScale;              // 4 bytes
    float _pad0;                            // 4 bytes = 16
    float3 g_CloudOffset;                   // 12 bytes
    float g_CloudDensityMultiplier;         // 4 bytes = 16
    float2 g_CloudWeatherOffset;            // 8 bytes
    float2 _pad1;                           // 8 bytes = 16
    
    // Row 15-16: Volumetric Cloud Density
    float g_CloudDensityBottom;             // 4 bytes
    float g_CloudDensityTop;                // 4 bytes
    float g_CloudHeightGradient;            // 4 bytes
    float g_CloudEdgeFade;                  // 4 bytes = 16
    float g_CloudCoverageRemapping;         // 4 bytes
    float g_CloudAnvilBias;                 // 4 bytes
    float g_CloudStorminess;                // 4 bytes
    float g_CloudRainAbsorption;            // 4 bytes = 16
    
    // Row 17-20: Volumetric Cloud Lighting
    float g_CloudScatteringCoeff;           // 4 bytes
    float g_CloudExtinctionCoeff;           // 4 bytes
    float g_CloudPowderCoeff;               // 4 bytes
    float g_CloudPhaseFunctionG;            // 4 bytes = 16
    float g_CloudMultiScatterStrength;      // 4 bytes
    float g_CloudAmbientStrength;           // 4 bytes
    float g_CloudSilverIntensity;           // 4 bytes
    float g_CloudSilverSpread;              // 4 bytes = 16
    float3 g_CloudAmbientColor;             // 12 bytes
    float g_NightSkyStrength;               // 4 bytes = 16
    float3 g_NightSkyColor;                 // 12 bytes
    float g_StarIntensity;                  // 4 bytes = 16
    
    // Row 21-22: Performance & Extras
    int g_CloudMaxSteps;                    // 4 bytes
    int g_CloudMaxLightSteps;               // 4 bytes
    float g_CloudStepSize;                  // 4 bytes
    float g_CloudMaxRenderDistance;         // 4 bytes = 16
    float g_CloudLodDistanceMultiplier;     // 4 bytes
    float g_CloudBlueNoiseScale;            // 4 bytes
    float g_MoonIntensity;                  // 4 bytes
    float g_OzoneStrength;                  // 4 bytes = 16
    
    // Row 23: Extra
    float g_AtmosphereDensityScale;         // 4 bytes
    float3 g_OzoneAbsorption;               // 12 bytes = 16
};

// ==================== SKY FLAGS ====================
#define SKY_FLAG_SUN_ENABLED              (1u << 0)
#define SKY_FLAG_SUN_DISK_ENABLED         (1u << 1)
#define SKY_FLAG_SUN_INTENSITY_AFFECTS    (1u << 2)
#define SKY_FLAG_CLOUDS_ENABLED           (1u << 3)
#define SKY_FLAG_CLOUDS_USE_SUN           (1u << 4)
#define SKY_FLAG_CLOUDS_CAST_SHADOWS      (1u << 5)
#define SKY_FLAG_FORCE_ATMOSPHERE_COLOR   (1u << 6)
#define SKY_FLAG_FORCE_HORIZON_COLOR      (1u << 7)
#define SKY_FLAG_FORCE_ZENITH_COLOR       (1u << 8)
#define SKY_FLAG_FORCE_CLOUD_COLOR        (1u << 9)
#define SKY_FLAG_FORCE_CLOUD_COVERAGE     (1u << 10)
#define SKY_FLAG_VOLUMETRIC_CLOUDS        (1u << 11)
#define SKY_FLAG_CLOUD_DETAIL_NOISE       (1u << 12)
#define SKY_FLAG_CLOUD_LIGHTING           (1u << 13)
#define SKY_FLAG_CLOUD_AMBIENT            (1u << 14)
#define SKY_FLAG_CLOUD_ATMOSPHERIC_PERSP  (1u << 15)
#define SKY_FLAG_CLOUD_POWDER             (1u << 16)
#define SKY_FLAG_CLOUD_SILVER_LINING      (1u << 17)

// ==================== QUALITY PRESETS ====================
#define QUALITY_LOW       0
#define QUALITY_MEDIUM    1
#define QUALITY_HIGH      2
#define QUALITY_ULTRA     3
#define QUALITY_CINEMATIC 4

// ==================== CONSTANTS ====================
static const float PI = 3.14159265359;
static const float3 RAYLEIGH_COEFFICIENTS = float3(5.5e-6, 13.0e-6, 22.4e-6);
)";

// ==================== Part 2: Noise Functions ====================
static const char* g_SkyPixelShaderSource_Part2 = R"(
// ==================== HASH & NOISE FUNCTIONS ====================
float hash(float n) { return frac(sin(n) * 43758.5453); }
float hash3to1(float3 p) { return frac(sin(dot(p, float3(127.1, 311.7, 74.7))) * 43758.5453); }

float3 hash3to3(float3 p)
{
    p = float3(dot(p, float3(127.1, 311.7, 74.7)),
               dot(p, float3(269.5, 183.3, 246.1)),
               dot(p, float3(113.5, 271.9, 124.6)));
    return frac(sin(p) * 43758.5453);
}

// 3D Gradient Noise
float gradientNoise3D(float3 p)
{
    float3 i = floor(p);
    float3 f = frac(p);
    float3 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
    
    float n000 = hash3to1(i);
    float n001 = hash3to1(i + float3(0, 0, 1));
    float n010 = hash3to1(i + float3(0, 1, 0));
    float n011 = hash3to1(i + float3(0, 1, 1));
    float n100 = hash3to1(i + float3(1, 0, 0));
    float n101 = hash3to1(i + float3(1, 0, 1));
    float n110 = hash3to1(i + float3(1, 1, 0));
    float n111 = hash3to1(i + float3(1, 1, 1));
    
    float n00 = lerp(n000, n100, u.x);
    float n01 = lerp(n001, n101, u.x);
    float n10 = lerp(n010, n110, u.x);
    float n11 = lerp(n011, n111, u.x);
    float n0 = lerp(n00, n10, u.y);
    float n1 = lerp(n01, n11, u.y);
    return lerp(n0, n1, u.z);
}

// Worley Noise for cloud billows
float worleyNoise3D(float3 p)
{
    float3 i = floor(p);
    float3 f = frac(p);
    float minDist = 1.0;
    
    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++)
    for (int z = -1; z <= 1; z++)
    {
        float3 neighbor = float3(x, y, z);
        float3 cellPoint = hash3to3(i + neighbor);
        float3 diff = neighbor + cellPoint - f;
        minDist = min(minDist, dot(diff, diff));
    }
    return sqrt(minDist);
}

// FBM 3D
float fbm3D(float3 p, int octaves)
{
    float value = 0.0, amplitude = 0.5, frequency = 1.0, maxValue = 0.0;
    for (int i = 0; i < octaves; i++)
    {
        value += amplitude * gradientNoise3D(p * frequency);
        maxValue += amplitude;
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return value / maxValue;
}

// Perlin-Worley hybrid
float perlinWorley3D(float3 p, int octaves)
{
    float perlin = fbm3D(p, octaves);
    float worley = 1.0 - worleyNoise3D(p);
    return saturate(perlin * 0.6 + worley * 0.4);
}

// 2D noise for fallback clouds
float noise2D(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash3to1(float3(i, 0));
    float b = hash3to1(float3(i + float2(1, 0), 0));
    float c = hash3to1(float3(i + float2(0, 1), 0));
    float d = hash3to1(float3(i + float2(1, 1), 0));
    return lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
}

float fbm2D(float2 p, int octaves)
{
    float value = 0.0, amplitude = 0.5, frequency = 1.0;
    for (int i = 0; i < octaves; i++)
    {
        value += amplitude * noise2D(p * frequency);
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return value;
}
)";

// ==================== Part 3: Atmosphere ====================
static const char* g_SkyPixelShaderSource_Part3 = R"(
// ==================== ATMOSPHERE SCATTERING ====================
float rayleighPhase(float cosTheta)
{
    return (3.0 / (16.0 * PI)) * (1.0 + cosTheta * cosTheta);
}

float miePhase(float cosTheta, float g)
{
    float g2 = g * g;
    float num = (1.0 - g2);
    float denom = pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5);
    return (3.0 / (8.0 * PI)) * num / max(denom, 0.0001);
}

float3 calculateAtmosphere(float3 viewDir, float3 sunDir)
{
    float sunDot = dot(viewDir, sunDir);
    float upDot = viewDir.y;
    
    // Rayleigh scattering (blue sky)
    float rayleigh = rayleighPhase(sunDot) * g_RayleighStrength;
    float3 rayleighColor = RAYLEIGH_COEFFICIENTS * rayleigh * 1000.0;
    
    // Mie scattering (sun glow)
    float mie = miePhase(sunDot, g_MieAnisotropy) * g_MieStrength;
    float3 mieColor = float3(1.0, 0.95, 0.8) * mie;
    
    // Height-based color blend
    float horizonBlend = 1.0 - saturate(upDot);
    horizonBlend = pow(horizonBlend, g_DensityFalloff);
    
    // Sun color based on height
    float3 sunColor = lerp(g_SunColorZenith, g_SunColorHorizon, horizonBlend);
    
    // Sky colors (use forced colors if flag set, otherwise compute)
    float3 zenithColor = (g_SkyFlags & SKY_FLAG_FORCE_ZENITH_COLOR) ? g_ZenithColor : float3(0.1, 0.3, 0.8);
    float3 horizonColor = (g_SkyFlags & SKY_FLAG_FORCE_HORIZON_COLOR) ? g_HorizonColor : float3(0.6, 0.7, 0.9);
    
    float3 baseColor = lerp(zenithColor, horizonColor, horizonBlend);
    
    // Ozone absorption (subtle blue/orange tint)
    float3 ozoneEffect = g_OzoneAbsorption * g_OzoneStrength * (1.0 - upDot) * 0.1;
    baseColor -= ozoneEffect;
    
    // Combine atmospheric effects
    float3 atmosphere = baseColor + rayleighColor + mieColor * sunColor;
    
    // Apply density scale and sun intensity
    atmosphere *= g_AtmosphereDensityScale;
    atmosphere *= max(g_SunIntensity * 0.5, 0.3);
    
    return atmosphere;
}

float3 calculateSunDisk(float3 viewDir, float3 sunDir)
{
    if (!(g_SkyFlags & SKY_FLAG_SUN_DISK_ENABLED))
        return float3(0, 0, 0);
    
    float sunDot = dot(viewDir, sunDir);
    float sunRadius = g_SunDiskSize * 0.02;
    float softness = g_SunDiskSoftness * 0.02;
    
    float sunDisk = smoothstep(cos(sunRadius + softness), cos(sunRadius - softness), sunDot);
    float3 sunColor = g_SunAverageColor * g_SunIntensity * sunDisk * 5.0;
    
    // Bloom effect
    float bloom = pow(max(sunDot, 0.0), 256.0 / max(g_SunBloomStrength, 0.01));
    sunColor += g_SunAverageColor * bloom * g_SunBloomStrength * 0.5;
    
    return sunColor;
}

float3 calculateNightSky(float3 viewDir, float nightBlend)
{
    if (nightBlend < 0.01) return float3(0, 0, 0);
    
    float3 nightColor = g_NightSkyColor * g_NightSkyStrength;
    
    // Simple procedural stars
    if (g_StarIntensity > 0.0)
    {
        float3 starCoord = viewDir * 500.0;
        float starNoise = hash3to1(floor(starCoord));
        float star = step(0.998, starNoise) * hash3to1(starCoord + 0.5);
        float twinkle = sin(g_Time * 3.0 + starNoise * 100.0) * 0.3 + 0.7;
        nightColor += float3(1, 1, 0.9) * star * g_StarIntensity * twinkle;
    }
    
    // Moon placeholder (simple bright spot)
    if (g_MoonIntensity > 0.0)
    {
        float3 moonDir = normalize(float3(0.5, 0.3, -0.8)); // Fixed moon position
        float moonDot = dot(viewDir, moonDir);
        float moonDisk = smoothstep(0.998, 0.999, moonDot);
        nightColor += float3(0.9, 0.9, 1.0) * moonDisk * g_MoonIntensity;
    }
    
    return nightColor * nightBlend;
}
)";

// ==================== Part 4: Cloud Density ====================
static const char* g_SkyPixelShaderSource_Part4 = R"(
// ==================== CLOUD DENSITY FUNCTIONS ====================
float getHeightFraction(float3 pos)
{
    return saturate((pos.y - g_CloudLayerBottom) / (g_CloudLayerTop - g_CloudLayerBottom));
}

float getHeightGradient(float heightFraction)
{
    // Cumulus-style gradient: thick at bottom, wispy at top
    float bottomGradient = smoothstep(0.0, 0.25, heightFraction);
    float topGradient = 1.0 - smoothstep(0.7, 1.0, heightFraction);
    float baseGradient = max(bottomGradient * topGradient, 0.1);
    
    // Anvil bias for cumulonimbus
    float anvilModifier = 1.0 + g_CloudAnvilBias * (1.0 - topGradient);
    
    return lerp(0.3, 1.0, baseGradient * anvilModifier * g_CloudHeightGradient);
}

float getWeatherSample(float3 pos)
{
    float2 weatherUV = pos.xz * g_CloudWeatherScale + g_CloudWeatherOffset;
    weatherUV += g_CloudDirection * g_CloudSpeed * g_Time * 0.1;
    
    // Multi-octave weather noise
    float weather = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    for (int i = 0; i < 3; i++)
    {
        weather += amplitude * gradientNoise3D(float3(weatherUV * frequency * 3.0, g_Time * 0.01 + i * 17.3));
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    weather = weather * 0.5 + 0.5;
    
    // Apply coverage
    float coverage = saturate(g_CloudCoverage + g_CloudStorminess * 0.3);
    float threshold = 1.0 - coverage;
    float softness = max(coverage * 0.5, 0.1);
    weather = saturate((weather - threshold) / softness);
    
    // Coverage remapping
    float remapPower = lerp(0.5, 1.5, g_CloudCoverageRemapping);
    return pow(weather, remapPower);
}

float sampleCloudDensity(float3 pos, bool cheap)
{
    float heightFraction = getHeightFraction(pos);
    if (heightFraction < -0.05 || heightFraction > 1.05) return 0.0;
    heightFraction = saturate(heightFraction);
    
    // Animated position
    float3 animatedPos = pos + g_CloudOffset;
    animatedPos.xz += g_CloudDirection * g_CloudSpeed * g_Time * 100.0;
    
    // Weather/coverage
    float weather = getWeatherSample(pos);
    float heightGradient = getHeightGradient(heightFraction);
    
    // Base shape noise
    float internalScale = g_CloudScale * 1000.0;
    float3 shapeCoord = animatedPos * internalScale;
    int shapeOctaves = cheap ? 2 : 4;
    float baseShape = perlinWorley3D(shapeCoord, shapeOctaves);
    baseShape = saturate(baseShape * 1.2);
    
    // Combine factors
    float baseDensity = baseShape * heightGradient;
    baseDensity = baseDensity * (0.5 + weather * 0.5);
    baseDensity *= (0.5 + g_CloudCoverage);
    
    if (cheap) return saturate(baseDensity * g_CloudDensityMultiplier);
    if (baseDensity < 0.01) return saturate(baseDensity * g_CloudDensityMultiplier);
    
    // Detail noise erosion
    if (g_SkyFlags & SKY_FLAG_CLOUD_DETAIL_NOISE)
    {
        float detailScale = g_CloudDetailScale * 1000.0;
        float3 detailCoord = animatedPos * detailScale + float3(g_Time * 0.5, 0, g_Time * 0.3);
        int detailOctaves = (g_VolumetricQuality >= QUALITY_ULTRA) ? 4 : 2;
        float detail = fbm3D(detailCoord, detailOctaves);
        
        // Apply curl noise influence
        float curlOffset = g_CloudCurliness * sin(detailCoord.x + detailCoord.z) * 0.1;
        detail += curlOffset;
        
        float erosionAmount = g_CloudErosion * 0.3 * (1.0 - heightFraction * 0.5);
        baseDensity = saturate(baseDensity - detail * erosionAmount);
    }
    
    // Height-based density variation
    float heightDensity = lerp(g_CloudDensityBottom, g_CloudDensityTop, heightFraction);
    baseDensity *= heightDensity;
    
    // Edge fade
    float edgeFade = smoothstep(0.0, 0.2, baseDensity);
    baseDensity *= lerp(1.0, edgeFade, g_CloudEdgeFade);
    
    return saturate(baseDensity * g_CloudDensityMultiplier * g_CloudDensity);
}
)";

// ==================== Part 5: Cloud Lighting ====================
static const char* g_SkyPixelShaderSource_Part5 = R"(
// ==================== CLOUD LIGHTING ====================
float henyeyGreenstein(float cosTheta, float g)
{
    float g2 = g * g;
    float denom = 1.0 + g2 - 2.0 * g * cosTheta;
    return (1.0 - g2) / (4.0 * PI * pow(max(denom, 0.0001), 1.5));
}

float dualLobePhase(float cosTheta, float g)
{
    float forward = henyeyGreenstein(cosTheta, g);
    float back = henyeyGreenstein(cosTheta, -g * 0.3);
    return lerp(back, forward, 0.7);
}

float powderEffect(float density, float cosTheta)
{
    float powder = 1.0 - exp(-density * g_CloudPowderCoeff);
    float beers = exp(-density);
    float blend = saturate(cosTheta * 0.5 + 0.5);
    return lerp(powder * beers, beers, blend);
}

float sampleLightEnergy(float3 pos, float3 sunDir, int samples)
{
    float stepSize = (g_CloudLayerTop - pos.y) / max(sunDir.y, 0.01) / float(samples);
    stepSize = min(stepSize, 500.0);
    
    float totalDensity = 0.0;
    float3 samplePos = pos;
    
    for (int i = 0; i < samples; i++)
    {
        samplePos += sunDir * stepSize;
        if (samplePos.y > g_CloudLayerTop) break;
        totalDensity += sampleCloudDensity(samplePos, true) * stepSize;
    }
    
    return exp(-totalDensity * g_CloudExtinctionCoeff);
}
)";

// ==================== Part 6: Ray Marching ====================
static const char* g_SkyPixelShaderSource_Part6 = R"(
// ==================== RAY MARCHING ====================
float2 rayCloudLayerIntersect(float3 rayOrigin, float3 rayDir)
{
    float2 result = float2(-1, -1);
    
    if (abs(rayDir.y) < 0.0001)
    {
        if (rayOrigin.y >= g_CloudLayerBottom && rayOrigin.y <= g_CloudLayerTop)
        {
            result.x = 0.0;
            result.y = 100000.0;
        }
        return result;
    }
    
    float tBottom = (g_CloudLayerBottom - rayOrigin.y) / rayDir.y;
    float tTop = (g_CloudLayerTop - rayOrigin.y) / rayDir.y;
    
    float tNear = min(tBottom, tTop);
    float tFar = max(tBottom, tTop);
    
    if (tFar < 0.0) return result;
    
    result.x = max(tNear, 0.0);
    result.y = max(tFar, 0.0);
    
    if (rayOrigin.y >= g_CloudLayerBottom && rayOrigin.y <= g_CloudLayerTop)
        result.x = 0.0;
    
    return result;
}

void getStepCounts(out int primarySteps, out int lightSteps)
{
    if (g_CloudMaxSteps > 0)
    {
        primarySteps = g_CloudMaxSteps;
        lightSteps = max(g_CloudMaxLightSteps, 2);
        return;
    }
    
    switch (g_VolumetricQuality)
    {
        case QUALITY_LOW:      primarySteps = 32;  lightSteps = 2;  break;
        case QUALITY_MEDIUM:   primarySteps = 64;  lightSteps = 4;  break;
        case QUALITY_HIGH:     primarySteps = 96;  lightSteps = 6;  break;
        case QUALITY_ULTRA:    primarySteps = 128; lightSteps = 8;  break;
        case QUALITY_CINEMATIC:primarySteps = 192; lightSteps = 12; break;
        default:               primarySteps = 64;  lightSteps = 4;  break;
    }
}

float4 rayMarchClouds(float3 rayOrigin, float3 rayDir, float3 sunDir, float maxDist)
{
    float2 intersection = rayCloudLayerIntersect(rayOrigin, rayDir);
    if (intersection.y <= 0.0 || intersection.x >= maxDist)
        return float4(0, 0, 0, 0);
    
    float tStart = max(intersection.x, 0.0);
    float tEnd = min(intersection.y, maxDist);
    if (tStart >= tEnd) return float4(0, 0, 0, 0);
    
    int primarySteps, lightSteps;
    getStepCounts(primarySteps, lightSteps);
    
    float stepSize = (tEnd - tStart) / float(primarySteps);
    stepSize = max(stepSize, g_CloudStepSize);
    
    // Blue noise jitter
    float jitter = hash3to1(rayDir * 1000.0 + g_Time) * stepSize * g_CloudBlueNoiseScale;
    
    // Phase functions
    float cosTheta = dot(rayDir, sunDir);
    float phase = dualLobePhase(cosTheta, g_CloudPhaseFunctionG);
    
    // Silver lining
    float silverPhase = 0.0;
    if (g_SkyFlags & SKY_FLAG_CLOUD_SILVER_LINING)
    {
        float silverCos = saturate(cosTheta);
        silverPhase = pow(silverCos, 1.0 / max(g_CloudSilverSpread, 0.01)) * g_CloudSilverIntensity;
    }
    
    // Accumulation
    float3 scatteredLight = float3(0, 0, 0);
    float transmittance = 1.0;
    
    // Sun color
    float sunHeight = saturate(-sunDir.y + 0.5);
    float3 sunColor = lerp(g_SunColorHorizon, g_SunColorZenith, sunHeight) * g_SunIntensity;
    float3 ambientColor = g_CloudAmbientColor * g_CloudAmbientStrength;
    
    float t = tStart + jitter;
    int zeroSamples = 0;
    
    for (int i = 0; i < primarySteps && t < tEnd && transmittance > 0.01; i++)
    {
        float3 pos = rayOrigin + rayDir * t;
        float density = sampleCloudDensity(pos, false);
        
        if (density > 0.001)
        {
            zeroSamples = 0;
            float heightFrac = getHeightFraction(pos);
            
            // Light sampling
            float lightEnergy = 1.0;
            if (g_SkyFlags & SKY_FLAG_CLOUD_LIGHTING)
                lightEnergy = sampleLightEnergy(pos, sunDir, lightSteps);
            
            // Powder effect
            float powder = 1.0;
            if (g_SkyFlags & SKY_FLAG_CLOUD_POWDER)
                powder = powderEffect(density * stepSize, cosTheta);
            
            // Multi-scattering
            float multiScatter = 1.0 + g_CloudMultiScatterStrength * (1.0 - lightEnergy);
            
            // Light contribution
            float3 lightContrib = sunColor * lightEnergy * phase * powder * multiScatter;
            lightContrib += sunColor * silverPhase * lightEnergy * 0.5;
            
            // Ambient
            if (g_SkyFlags & SKY_FLAG_CLOUD_AMBIENT)
            {
                float ao = 1.0 - density * 0.5;
                lightContrib += ambientColor * ao;
            }
            
            // Cloud color
            float3 cloudTint = (g_SkyFlags & SKY_FLAG_FORCE_CLOUD_COLOR) ? g_CloudColor : float3(1, 1, 1);
            lightContrib *= cloudTint;
            
            // Rain darkening
            lightContrib *= 1.0 - g_CloudRainAbsorption * g_CloudStorminess;
            
            // Beer's law
            float sampleExtinction = density * g_CloudExtinctionCoeff * stepSize;
            float sampleTransmittance = exp(-sampleExtinction);
            
            float3 integScatter = lightContrib * (1.0 - sampleTransmittance) / max(g_CloudExtinctionCoeff, 0.0001);
            scatteredLight += transmittance * integScatter;
            transmittance *= sampleTransmittance;
        }
        else
        {
            zeroSamples++;
            if (zeroSamples > 4) t += stepSize;
        }
        
        t += stepSize;
    }
    
    float alpha = 1.0 - transmittance;
    
    // Atmospheric perspective
    if (g_SkyFlags & SKY_FLAG_CLOUD_ATMOSPHERIC_PERSP)
    {
        float atmosphereFade = 1.0 - exp(-tStart * 0.00001);
        scatteredLight = lerp(scatteredLight, g_HorizonColor * 0.5, atmosphereFade * 0.5);
    }
    
    return float4(scatteredLight, alpha);
}
)";

// ==================== Part 7: 2D Clouds & Main ====================
static const char* g_SkyPixelShaderSource_Part7 = R"(
// ==================== 2D CLOUDS (FALLBACK) ====================
float3 calculate2DClouds(float3 viewDir, float3 sunDir)
{
    if (!(g_SkyFlags & SKY_FLAG_CLOUDS_ENABLED)) return float3(0, 0, 0);
    if (viewDir.y <= 0.005) return float3(0, 0, 0);
    
    float t = g_CloudHeight / max(viewDir.y, 0.01);
    float2 cloudUV = viewDir.xz * t * 0.0002;
    float2 windOffset = normalize(g_CloudDirection + float2(0.001, 0.001)) * g_CloudSpeed * g_Time;
    cloudUV += windOffset;
    
    int octaves = 3 + (int)g_SkyQuality;
    float cloudNoise = fbm2D(cloudUV * 4.0, octaves);
    
    float coverage = saturate(g_CloudCoverage);
    float lowerThreshold = 1.0 - coverage;
    float upperThreshold = 1.0 - coverage * 0.3;
    float cloudShape = smoothstep(lowerThreshold, upperThreshold, cloudNoise);
    float density = cloudShape * g_CloudDensity;
    density = max(density, cloudNoise * coverage * 0.3);
    
    float3 cloudColor = (g_SkyFlags & SKY_FLAG_FORCE_CLOUD_COLOR) ? g_CloudColor : float3(1, 1, 1);
    
    if (g_SkyFlags & SKY_FLAG_CLOUDS_USE_SUN)
    {
        float sunHeight = saturate(-sunDir.y + 0.5);
        float3 sunTint = lerp(g_SunColorHorizon, g_SunColorZenith, sunHeight);
        cloudColor *= lerp(float3(1, 1, 1), sunTint * g_SunIntensity, g_CloudSunLightInfluence);
    }
    
    float shadow = 1.0 - g_CloudLightAbsorption * density * 0.3;
    cloudColor *= max(shadow, 0.3);
    
    float horizonFade = saturate(viewDir.y * 2.0);
    density *= horizonFade;
    
    return cloudColor * density;
}

// ==================== PIXEL SHADER ====================
struct PS_INPUT
{
    float4 position : SV_POSITION;
    float3 viewDir : TEXCOORD0;
    float2 uv : TEXCOORD1;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    float3 viewDir = normalize(input.viewDir);
    float3 sunDir = normalize(-g_SunDirection);
    
    // Atmosphere
    float3 atmosphere = float3(0, 0, 0);
    if (g_SkyFlags & SKY_FLAG_SUN_ENABLED)
        atmosphere = calculateAtmosphere(viewDir, sunDir);
    else
    {
        float upDot = saturate(viewDir.y);
        atmosphere = lerp(g_HorizonColor, g_ZenithColor, upDot);
    }
    
    // Clouds
    float3 clouds = float3(0, 0, 0);
    float cloudAlpha = 0.0;
    
    if (g_SkyFlags & SKY_FLAG_CLOUDS_ENABLED)
    {
        if ((g_SkyFlags & SKY_FLAG_VOLUMETRIC_CLOUDS) && viewDir.y > 0.005)
        {
            float3 rayOrigin = float3(0, 100, 0);
            float4 cloudResult = rayMarchClouds(rayOrigin, viewDir, sunDir, g_CloudMaxRenderDistance);
            clouds = cloudResult.rgb;
            cloudAlpha = cloudResult.a;
            
            if (cloudAlpha < 0.001)
            {
                clouds = calculate2DClouds(viewDir, sunDir);
                cloudAlpha = saturate(length(clouds) * 1.5);
            }
        }
        else if (viewDir.y > 0.0)
        {
            clouds = calculate2DClouds(viewDir, sunDir);
            cloudAlpha = saturate(length(clouds) * 1.5);
        }
    }
    
    // Sun disk
    float3 sunDisk = calculateSunDisk(viewDir, sunDir);
    
    // Night sky
    float nightBlend = saturate(-sunDir.y * 2.0);
    float3 nightSky = calculateNightSky(viewDir, nightBlend);
    
    // Composite
    float3 finalColor = atmosphere;
    finalColor = lerp(finalColor, clouds, cloudAlpha * 0.9);
    finalColor += sunDisk * (1.0 - cloudAlpha * 0.7);
    finalColor = lerp(finalColor, finalColor * 0.1 + nightSky, nightBlend * (1.0 - cloudAlpha));
    
    // ACES tone mapping
    float3 x = finalColor;
    float3 a = x * (x + 0.0245786) - 0.000090537;
    float3 b = x * (0.983729 * x + 0.4329510) + 0.238081;
    finalColor = a / b;
    
    // Gamma
    finalColor = pow(max(finalColor, 0.0), 1.0 / 2.2);
    
    return float4(finalColor, 1.0);
}
)";

// ==================== Combined Shader Source ====================
static const std::string g_SkyPixelShaderSource = 
    std::string(g_SkyPixelShaderSource_Part1) +
    std::string(g_SkyPixelShaderSource_Part2) +
    std::string(g_SkyPixelShaderSource_Part3) +
    std::string(g_SkyPixelShaderSource_Part4) +
    std::string(g_SkyPixelShaderSource_Part5) +
    std::string(g_SkyPixelShaderSource_Part6) +
    std::string(g_SkyPixelShaderSource_Part7);
