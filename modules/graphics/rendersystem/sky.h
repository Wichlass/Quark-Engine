
#pragma once

#include "../../headeronly/globaltypes.h"
#include "../../headeronly/mathematics.h"

// ==================== SKY FLAGS ====================
enum class SkyFlags : UINT32
{
    NONE = 0,
    SUN_ENABLED = 1 << 0,
    SUN_DISK_ENABLED = 1 << 1,
    SUN_INTENSITY_AFFECTS_COLOR = 1 << 2,
    CLOUDS_ENABLED = 1 << 3,
    CLOUDS_USE_SUN_LIGHT = 1 << 4,
    CLOUDS_CAST_SHADOWS = 1 << 5,               // NOT IMPLEMENTED - Reserved
    FORCE_ATMOSPHERE_COLOR = 1 << 6,
    FORCE_HORIZON_COLOR = 1 << 7,
    FORCE_ZENITH_COLOR = 1 << 8,
    FORCE_CLOUD_COLOR = 1 << 9,
    FORCE_CLOUD_COVERAGE = 1 << 10,
    // NEW: Volumetric cloud flags
    VOLUMETRIC_CLOUDS = 1 << 11,                // Enable 3D volumetric ray marching
    CLOUD_DETAIL_NOISE = 1 << 12,               // Use detail noise for erosion
    CLOUD_LIGHTING_ENABLED = 1 << 13,           // Enable cloud lighting calculations
    CLOUD_AMBIENT_ENABLED = 1 << 14,            // Enable ambient lighting on clouds
    CLOUD_ATMOSPHERIC_PERSPECTIVE = 1 << 15,    // Fade clouds with distance
    CLOUD_POWDER_EFFECT = 1 << 16,              // Dark edges powder effect
    CLOUD_SILVER_LINING = 1 << 17,              // Silver lining around sun
    ATMOSPHERE_ADVANCED = 1 << 18,              // Use advanced atmosphere model
    STARS_ENABLED = 1 << 19,                    // Enable star field at night
    MOON_ENABLED = 1 << 20                      // Enable moon rendering
};

inline SkyFlags operator|(SkyFlags a, SkyFlags b)
{
    return static_cast<SkyFlags>(static_cast<UINT32>(a) | static_cast<UINT32>(b));
}

inline SkyFlags operator&(SkyFlags a, SkyFlags b)
{
    return static_cast<SkyFlags>(static_cast<UINT32>(a) & static_cast<UINT32>(b));
}

inline SkyFlags operator^(SkyFlags a, SkyFlags b)
{
    return static_cast<SkyFlags>(static_cast<UINT32>(a) ^ static_cast<UINT32>(b));
}

inline SkyFlags operator~(SkyFlags a)
{
    return static_cast<SkyFlags>(~static_cast<UINT32>(a));
}

inline SkyFlags& operator|=(SkyFlags& a, SkyFlags b)
{
    a = a | b;
    return a;
}

inline SkyFlags& operator&=(SkyFlags& a, SkyFlags b)
{
    a = a & b;
    return a;
}

inline SkyFlags& operator^=(SkyFlags& a, SkyFlags b)
{
    a = a ^ b;
    return a;
}

// ==================== SKY QUALITY ====================
enum class SkyQuality : UINT32
{
    LOW = 0,
    MEDIUM = 1,
    HIGH = 2,
    ULTRA = 3
};

inline SkyQuality operator|(SkyQuality a, SkyQuality b)
{
    return static_cast<SkyQuality>(static_cast<UINT32>(a) | static_cast<UINT32>(b));
}

inline SkyQuality operator&(SkyQuality a, SkyQuality b)
{
    return static_cast<SkyQuality>(static_cast<UINT32>(a) & static_cast<UINT32>(b));
}

inline SkyQuality operator^(SkyQuality a, SkyQuality b)
{
    return static_cast<SkyQuality>(static_cast<UINT32>(a) ^ static_cast<UINT32>(b));
}

inline SkyQuality operator~(SkyQuality a)
{
    return static_cast<SkyQuality>(~static_cast<UINT32>(a));
}

inline SkyQuality& operator|=(SkyQuality& a, SkyQuality b)
{
    a = a | b;
    return a;
}

inline SkyQuality& operator&=(SkyQuality& a, SkyQuality b)
{
    a = a & b;
    return a;
}

inline SkyQuality& operator^=(SkyQuality& a, SkyQuality b)
{
    a = a ^ b;
    return a;
}

// ==================== GPU SKY DATA ====================
// GPU-aligned sky constant buffer structure
// This struct is sent directly to the GPU - must be 16-byte aligned
// Each row = 16 bytes: Vec3+float OR 4xfloat OR Vec2+Vec2
struct GPUSkyData
{
    // Row 0: Flags & Quality
    UINT32 flags;                           // 4 bytes
    UINT32 quality;                         // 4 bytes
    UINT32 volumetricQuality;               // 4 bytes
    float time;                             // 4 bytes = 16
    
    // Row 1-4: Sun
    Quark::Vec3 sunDirection;               // 12 bytes
    float sunIntensity;                     // 4 bytes = 16
    Quark::Vec3 sunColorZenith;             // 12 bytes
    float sunDiskSize;                      // 4 bytes = 16
    Quark::Vec3 sunColorHorizon;            // 12 bytes
    float sunDiskSoftness;                  // 4 bytes = 16
    Quark::Vec3 sunAverageColor;            // 12 bytes
    float sunBloomStrength;                 // 4 bytes = 16
    
    // Row 5-7: Atmosphere
    float rayleighStrength;                 // 4 bytes
    float mieStrength;                      // 4 bytes
    float mieAnisotropy;                    // 4 bytes
    float atmosphereHeight;                 // 4 bytes = 16
    Quark::Vec3 horizonColor;               // 12 bytes
    float densityFalloff;                   // 4 bytes = 16
    Quark::Vec3 zenithColor;                // 12 bytes
    float atmospherePlanetRadius;           // 4 bytes = 16
    
    // Row 8-10: Cloud 2D
    float cloudCoverage;                    // 4 bytes
    float cloudDensity;                     // 4 bytes
    float cloudHeight;                      // 4 bytes
    float cloudThickness;                   // 4 bytes = 16
    float cloudSpeed;                       // 4 bytes
    float cloudSunLightInfluence;           // 4 bytes
    Quark::Vec2 cloudDirection;             // 8 bytes = 16
    Quark::Vec3 cloudColor;                 // 12 bytes
    float cloudLightAbsorption;             // 4 bytes = 16
    
    // Row 11-14: Volumetric Cloud Shape
    float cloudLayerBottom;                 // 4 bytes
    float cloudLayerTop;                    // 4 bytes
    float cloudScale;                       // 4 bytes
    float cloudDetailScale;                 // 4 bytes = 16
    float cloudErosion;                     // 4 bytes
    float cloudCurliness;                   // 4 bytes
    float cloudWeatherScale;                // 4 bytes
    float _pad0;                            // 4 bytes = 16
    Quark::Vec3 cloudOffset;                // 12 bytes
    float cloudDensityMultiplier;           // 4 bytes = 16
    Quark::Vec2 cloudWeatherOffset;         // 8 bytes
    Quark::Vec2 _pad1;                      // 8 bytes = 16
    
    // Row 15-16: Volumetric Cloud Density
    float cloudDensityBottom;               // 4 bytes
    float cloudDensityTop;                  // 4 bytes
    float cloudHeightGradient;              // 4 bytes
    float cloudEdgeFade;                    // 4 bytes = 16
    float cloudCoverageRemapping;           // 4 bytes
    float cloudAnvilBias;                   // 4 bytes
    float cloudStorminess;                  // 4 bytes
    float cloudRainAbsorption;              // 4 bytes = 16
    
    // Row 17-20: Volumetric Cloud Lighting
    float cloudScatteringCoeff;             // 4 bytes
    float cloudExtinctionCoeff;             // 4 bytes
    float cloudPowderCoeff;                 // 4 bytes
    float cloudPhaseFunctionG;              // 4 bytes = 16
    float cloudMultiScatterStrength;        // 4 bytes
    float cloudAmbientStrength;             // 4 bytes
    float cloudSilverIntensity;             // 4 bytes
    float cloudSilverSpread;                // 4 bytes = 16
    Quark::Vec3 cloudAmbientColor;          // 12 bytes
    float nightSkyStrength;                 // 4 bytes = 16
    Quark::Vec3 nightSkyColor;              // 12 bytes
    float starIntensity;                    // 4 bytes = 16
    
    // Row 21-22: Performance & Extras
    int cloudMaxSteps;                      // 4 bytes
    int cloudMaxLightSteps;                 // 4 bytes
    float cloudStepSize;                    // 4 bytes
    float cloudMaxRenderDistance;           // 4 bytes = 16
    float cloudLodDistanceMultiplier;       // 4 bytes
    float cloudBlueNoiseScale;              // 4 bytes
    float moonIntensity;                    // 4 bytes
    float ozoneStrength;                    // 4 bytes = 16
    
    // Row 23: Extra
    float atmosphereDensityScale;           // 4 bytes
    Quark::Vec3 ozoneAbsorption;            // 12 bytes = 16
};


// ==================== VOLUMETRIC CLOUD QUALITY ====================
enum class VolumetricCloudQuality : UINT32
{
    LOW = 0,       // 32 samples, 2 light samples - Mobile/Low-end
    MEDIUM = 1,    // 64 samples, 4 light samples - Balanced
    HIGH = 2,      // 128 samples, 6 light samples - High quality
    ULTRA = 3,     // 192 samples, 8 light samples - Ultra, detail noise
    CINEMATIC = 4  // 256 samples, 12 light samples - Maximum quality
};

// ==================== SKY SETTINGS ====================
struct SkySettings
{
    SkyFlags flags;                   // bayraklar
    SkyQuality quality;               // Sky kalitesi

    // ==================== SUN / GÜNEŞ ====================
    Quark::Vec3 sunDirection;         // Güneş yönü (normalizasyonlu)
    float sunIntensity;               // Güneş ışık gücü (PBR ile uyumlu)
    Quark::Vec3 sunColorZenith;       // Güneş dikken gökyüzüne verdiği renk
    Quark::Vec3 sunColorHorizon;      // Güneş yataydayken gökyüzüne verdiği renk
    Quark::Vec3 sunAverageColor;      // Güneş için ortalama renk (blend veya artist override)
    float sunDiskSize;                // Güneş küresinin boyutu (açısal büyüklük)
    float sunDiskSoftness;            // Güneş küresi kenar yumuşaklığı

    // ==================== ATMOSPHERE / ATMOSFER ====================
    float rayleighStrength;           // Rayleigh saçılım kuvveti (mavi gökyüzü)
    float mieStrength;                // Mie saçılım kuvveti (güneş etrafı parlaması)
    float mieAnisotropy;              // Mie saçılım yönlülüğü (0–1 arası)
    Quark::Vec3 horizonColor;         // Ufuk rengi
    Quark::Vec3 zenithColor;          // Tepede gökyüzü rengi
    float atmosphereHeight;           // Atmosferin yüksekliği
    float densityFalloff;             // Atmosfer yoğunluğunun yükseklikle azalması

    // ==================== CLOUDS / BULUTLAR (2D Procedural) ====================
    float cloudCoverage;              // Bulut oranı (0 = hiç, 1 = tamamen kapalı)
    float cloudDensity;               // Bulut hacim yoğunluğu
    float cloudHeight;                // Bulut katmanı yüksekliği
    float cloudThickness;             // Bulut kalınlığı / volumetrik hissi
    float cloudSpeed;                 // Bulutların hareket hızı
    Quark::Vec2 cloudDirection;       // Bulut rüzgar yönü (x = yatay, y = dikey)
    float cloudSunLightInfluence;     // Güneş ışığının bulutları aydınlatma oranı
    float cloudLightAbsorption;       // Bulutların gölge sertliği
    Quark::Vec3 cloudColor;           // Bulutların ortalama rengi

    // ==================== VOLUMETRIC CLOUDS - SHAPE ====================
    VolumetricCloudQuality volumetricQuality;  // Ray march kalite preset
    float cloudLayerBottom;           // Alt bulut yüksekliği (metre), default 1500
    float cloudLayerTop;              // Üst bulut yüksekliği (metre), default 4000
    float cloudScale;                 // Base noise scale, default 0.0003
    float cloudDetailScale;           // Detail noise scale, default 0.003
    float cloudErosion;               // Detail erosion strength 0-1, default 0.5
    float cloudCurliness;             // Curl noise strength, default 0.3
    Quark::Vec3 cloudOffset;          // World-space offset for animation

    // ==================== VOLUMETRIC CLOUDS - DENSITY ====================
    float cloudDensityMultiplier;     // Global density multiplier, default 1.0
    float cloudDensityBottom;         // Density at cloud base, default 1.0
    float cloudDensityTop;            // Density at cloud top, default 0.2
    float cloudHeightGradient;        // Height-based density falloff, default 0.8
    float cloudEdgeFade;              // Edge softness, default 0.3
    float cloudCoverageRemapping;     // Remap coverage values, default 0.5
    float cloudAnvilBias;             // Anvil shape bias at top, default 0.0

    // ==================== VOLUMETRIC CLOUDS - LIGHTING ====================
    float cloudScatteringCoeff;       // Scattering coefficient, default 0.05
    float cloudExtinctionCoeff;       // Extinction coefficient, default 0.04
    float cloudPowderCoeff;           // Powder effect strength (dark edges), default 2.0
    float cloudPhaseFunctionG;        // HG phase g parameter (forward scatter), default 0.8
    float cloudMultiScatterStrength;  // Multi-scatter approximation, default 0.5
    float cloudAmbientStrength;       // Ambient light contribution, default 0.3
    float cloudSilverIntensity;       // Silver lining effect intensity, default 0.8
    float cloudSilverSpread;          // Silver lining spread, default 0.15
    Quark::Vec3 cloudAmbientColor;    // Sky ambient for clouds

    // ==================== VOLUMETRIC CLOUDS - WEATHER ====================
    float cloudWeatherScale;          // Weather map scale, default 0.00005
    Quark::Vec2 cloudWeatherOffset;   // Weather map animation offset
    float cloudStorminess;            // Storm cloud density boost, default 0.0
    float cloudRainAbsorption;        // Rain cloud darkness, default 0.5

    // ==================== VOLUMETRIC CLOUDS - PERFORMANCE ====================
    int cloudMaxSteps;                // Max ray march steps (0 = auto from quality)
    int cloudMaxLightSteps;           // Max light samples (0 = auto from quality)
    float cloudStepSize;              // Base step size (meters), default 100.0
    float cloudMaxRenderDistance;     // Max render distance, default 150000
    float cloudLodDistanceMultiplier; // LOD transition distance, default 1.0
    float cloudBlueNoiseScale;        // Jitter scale for banding reduction, default 1.0

    // ==================== ATMOSPHERE - ADVANCED ====================
    float atmospherePlanetRadius;     // Planet radius (meters), default 6371000
    float atmosphereDensityScale;     // Atmosphere density scale, default 1.0
    float ozoneStrength;              // Ozone absorption strength, default 0.0
    Quark::Vec3 ozoneAbsorption;      // Ozone absorption color
    float nightSkyStrength;           // Night sky brightness, default 0.01
    Quark::Vec3 nightSkyColor;        // Night sky tint
    float sunBloomStrength;           // Sun bloom spread, default 0.5
    float starIntensity;              // Star field intensity, default 0.5
    float moonIntensity;              // Moon brightness, default 0.3

    // Default constructor with sensible values
    SkySettings()
        : flags(SkyFlags::SUN_ENABLED | SkyFlags::SUN_DISK_ENABLED | SkyFlags::CLOUDS_ENABLED 
              | SkyFlags::CLOUDS_USE_SUN_LIGHT 
              | SkyFlags::CLOUD_DETAIL_NOISE | SkyFlags::CLOUD_LIGHTING_ENABLED
              | SkyFlags::CLOUD_AMBIENT_ENABLED | SkyFlags::CLOUD_ATMOSPHERIC_PERSPECTIVE
              | SkyFlags::CLOUD_POWDER_EFFECT | SkyFlags::CLOUD_SILVER_LINING)
        , quality(SkyQuality::HIGH)
        // Sun defaults
        , sunDirection(Quark::Vec3(-0.5f, -0.7f, -0.5f).Normalized())
        , sunIntensity(1.5f)
        , sunColorZenith(1.0f, 0.98f, 0.95f)
        , sunColorHorizon(1.0f, 0.6f, 0.3f)
        , sunAverageColor(1.0f, 0.95f, 0.8f)
        , sunDiskSize(3.0f)
        , sunDiskSoftness(1.0f)
        // Atmosphere defaults
        , rayleighStrength(1.0f)
        , mieStrength(0.5f)
        , mieAnisotropy(0.8f)
        , horizonColor(0.7f, 0.8f, 0.95f)
        , zenithColor(0.15f, 0.35f, 0.8f)
        , atmosphereHeight(10000.0f)
        , densityFalloff(4.0f)
        // Cloud 2D defaults
        , cloudCoverage(0.5f)
        , cloudDensity(0.8f)
        , cloudHeight(1500.0f)
        , cloudThickness(800.0f)
        , cloudSpeed(0.02f)
        , cloudDirection(1.0f, 0.0f)
        , cloudSunLightInfluence(0.8f)
        , cloudLightAbsorption(0.3f)
        , cloudColor(1.0f, 1.0f, 1.0f)
        // Volumetric cloud shape defaults
        , volumetricQuality(VolumetricCloudQuality::HIGH)
        , cloudLayerBottom(1500.0f)
        , cloudLayerTop(4000.0f)
        , cloudScale(0.00025f)
        , cloudDetailScale(0.002f)
        , cloudErosion(0.5f)
        , cloudCurliness(0.3f)
        , cloudOffset(0.0f, 0.0f, 0.0f)
        // Volumetric cloud density defaults
        , cloudDensityMultiplier(1.0f)
        , cloudDensityBottom(1.0f)
        , cloudDensityTop(0.2f)
        , cloudHeightGradient(0.7f)
        , cloudEdgeFade(0.3f)
        , cloudCoverageRemapping(0.5f)
        , cloudAnvilBias(0.0f)
        // Volumetric cloud lighting defaults
        , cloudScatteringCoeff(0.05f)
        , cloudExtinctionCoeff(0.04f)
        , cloudPowderCoeff(2.0f)
        , cloudPhaseFunctionG(0.8f)
        , cloudMultiScatterStrength(0.5f)
        , cloudAmbientStrength(0.35f)
        , cloudSilverIntensity(0.8f)
        , cloudSilverSpread(0.15f)
        , cloudAmbientColor(0.6f, 0.7f, 0.9f)
        // Volumetric cloud weather defaults
        , cloudWeatherScale(0.00005f)
        , cloudWeatherOffset(0.0f, 0.0f)
        , cloudStorminess(0.0f)
        , cloudRainAbsorption(0.5f)
        // Volumetric cloud performance defaults
        , cloudMaxSteps(0)
        , cloudMaxLightSteps(0)
        , cloudStepSize(100.0f)
        , cloudMaxRenderDistance(150000.0f)
        , cloudLodDistanceMultiplier(1.0f)
        , cloudBlueNoiseScale(1.0f)
        // Atmosphere advanced defaults
        , atmospherePlanetRadius(6371000.0f)
        , atmosphereDensityScale(1.0f)
        , ozoneStrength(0.0f)
        , ozoneAbsorption(0.0f, 0.0f, 0.0f)
        , nightSkyStrength(0.01f)
        , nightSkyColor(0.02f, 0.03f, 0.08f)
        , sunBloomStrength(0.5f)
        , starIntensity(0.5f)
        , moonIntensity(0.3f)
    {}

    // Convert to GPU constant buffer format
    GPUSkyData toGPU(float time) const
    {
        GPUSkyData gpu{};
        
        // Row 0: Flags & Quality
        gpu.flags = static_cast<UINT32>(flags);
        gpu.quality = static_cast<UINT32>(quality);
        gpu.volumetricQuality = static_cast<UINT32>(volumetricQuality);
        gpu.time = time;
        
        // Row 1-4: Sun
        gpu.sunDirection = sunDirection;
        gpu.sunIntensity = sunIntensity;
        gpu.sunColorZenith = sunColorZenith;
        gpu.sunDiskSize = sunDiskSize;
        gpu.sunColorHorizon = sunColorHorizon;
        gpu.sunDiskSoftness = sunDiskSoftness;
        gpu.sunAverageColor = sunAverageColor;
        gpu.sunBloomStrength = sunBloomStrength;
        
        // Row 5-7: Atmosphere
        gpu.rayleighStrength = rayleighStrength;
        gpu.mieStrength = mieStrength;
        gpu.mieAnisotropy = mieAnisotropy;
        gpu.atmosphereHeight = atmosphereHeight;
        gpu.horizonColor = horizonColor;
        gpu.densityFalloff = densityFalloff;
        gpu.zenithColor = zenithColor;
        gpu.atmospherePlanetRadius = atmospherePlanetRadius;
        
        // Row 8-10: Cloud 2D
        gpu.cloudCoverage = cloudCoverage;
        gpu.cloudDensity = cloudDensity;
        gpu.cloudHeight = cloudHeight;
        gpu.cloudThickness = cloudThickness;
        gpu.cloudSpeed = cloudSpeed;
        gpu.cloudSunLightInfluence = cloudSunLightInfluence;
        gpu.cloudDirection = cloudDirection;
        gpu.cloudColor = cloudColor;
        gpu.cloudLightAbsorption = cloudLightAbsorption;
        
        // Row 11-14: Volumetric Cloud Shape
        gpu.cloudLayerBottom = cloudLayerBottom;
        gpu.cloudLayerTop = cloudLayerTop;
        gpu.cloudScale = cloudScale;
        gpu.cloudDetailScale = cloudDetailScale;
        gpu.cloudErosion = cloudErosion;
        gpu.cloudCurliness = cloudCurliness;
        gpu.cloudWeatherScale = cloudWeatherScale;
        gpu.cloudOffset = cloudOffset;
        gpu.cloudDensityMultiplier = cloudDensityMultiplier;
        gpu.cloudWeatherOffset = cloudWeatherOffset;
        
        // Row 15-16: Volumetric Cloud Density
        gpu.cloudDensityBottom = cloudDensityBottom;
        gpu.cloudDensityTop = cloudDensityTop;
        gpu.cloudHeightGradient = cloudHeightGradient;
        gpu.cloudEdgeFade = cloudEdgeFade;
        gpu.cloudCoverageRemapping = cloudCoverageRemapping;
        gpu.cloudAnvilBias = cloudAnvilBias;
        gpu.cloudStorminess = cloudStorminess;
        gpu.cloudRainAbsorption = cloudRainAbsorption;
        
        // Row 17-20: Volumetric Cloud Lighting
        gpu.cloudScatteringCoeff = cloudScatteringCoeff;
        gpu.cloudExtinctionCoeff = cloudExtinctionCoeff;
        gpu.cloudPowderCoeff = cloudPowderCoeff;
        gpu.cloudPhaseFunctionG = cloudPhaseFunctionG;
        gpu.cloudMultiScatterStrength = cloudMultiScatterStrength;
        gpu.cloudAmbientStrength = cloudAmbientStrength;
        gpu.cloudSilverIntensity = cloudSilverIntensity;
        gpu.cloudSilverSpread = cloudSilverSpread;
        gpu.cloudAmbientColor = cloudAmbientColor;
        gpu.nightSkyStrength = nightSkyStrength;
        gpu.nightSkyColor = nightSkyColor;
        gpu.starIntensity = starIntensity;
        
        // Row 21-22: Performance & Extras
        gpu.cloudMaxSteps = cloudMaxSteps;
        gpu.cloudMaxLightSteps = cloudMaxLightSteps;
        gpu.cloudStepSize = cloudStepSize;
        gpu.cloudMaxRenderDistance = cloudMaxRenderDistance;
        gpu.cloudLodDistanceMultiplier = cloudLodDistanceMultiplier;
        gpu.cloudBlueNoiseScale = cloudBlueNoiseScale;
        gpu.moonIntensity = moonIntensity;
        gpu.ozoneStrength = ozoneStrength;
        
        // Row 23: Extra
        gpu.atmosphereDensityScale = atmosphereDensityScale;
        gpu.ozoneAbsorption = ozoneAbsorption;
        
        return gpu;
    }
};