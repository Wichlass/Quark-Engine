#pragma once

#include <vector>
#include "../../headeronly/globaltypes.h"
#include "../../headeronly/mathematics.h"
#include "rstypes.h"
#include "camera.h"
#include "material.h"
#include "lighting.h"
#include "csm.h"
#include "sky.h"

// ==================== DRAW COMMAND ====================
struct DrawCommand
{
    hMesh mesh;
    hMaterial material;
    UINT32 instanceStart;
    UINT32 instanceCount;
    UINT32 sortKey;
};

// ==================== PER-INSTANCE DATA ====================
struct PerInstanceData
{
    Quark::Mat4 worldMatrix;
    Quark::Mat4 worldInvTranspose;
    Quark::Vec4 customData;
};


// ==================== FRAME CONSTANTS ====================
struct FrameConstants
{
    Quark::Mat4 view;
    Quark::Mat4 projection;
    Quark::Mat4 viewProjection;
    Quark::Mat4 invView;
    Quark::Mat4 invProjection;
    
    Quark::Vec3 cameraPosition;
    float time;
    
    Quark::Color ambientLight;
    
    float deltaTime;
    UINT32 activeLightCount;
    UINT32 shadowAtlasSize;       // Directional CSM atlas size
    UINT32 localShadowAtlasSize;  // Local light (spot/point) atlas size
    // 16-byte aligned: deltaTime(4) + 3 UINT32s(12) = 16 bytes
};

// ==================== FRAME PACKET ====================
struct FramePacket
{
    FrameConstants constants;
    float clearColor[4];
    
    DrawCommand* drawCommands;
    UINT32 drawCommandCount;
    
    DrawCommand* shadowDrawCommands;
    UINT32 shadowDrawCommandCount;
    
    PerInstanceData* instanceData;
    UINT32 instanceDataCount;
    
    PerInstanceData* shadowInstanceData;
    UINT32 shadowInstanceDataCount;
    
    MaterialData* materials;
    hMaterial* materialHandles;
    UINT32 materialCount;
    
    GPULightData* lights;
    UINT32 lightCount;
    
    UINT32 viewportWidth;
    UINT32 viewportHeight;
    
    SkySettings skySettings;
};

// ==================== FRAME PACKET BUILDER ====================
class FramePacketBuilder
{
private:
    static constexpr UINT32 MAX_DRAW_COMMANDS = 16384;
    static constexpr UINT32 MAX_INSTANCES = 65536;
    static constexpr UINT32 MAX_MATERIALS = 1024;
    
    std::vector<DrawCommand> m_DrawCommands;
    std::vector<PerInstanceData> m_InstanceData;
    std::vector<DrawCommand> m_ShadowDrawCommands;
    std::vector<PerInstanceData> m_ShadowInstanceData;
    std::vector<MaterialData> m_Materials;
    std::vector<hMaterial> m_MaterialHandles;
    std::vector<GPULightData> m_Lights;
    
    UINT32 m_DrawCommandCount;
    UINT32 m_InstanceCount;
    UINT32 m_ShadowDrawCommandCount;
    UINT32 m_ShadowInstanceCount;
    UINT32 m_MaterialCount;
    UINT32 m_LightCount;
    UINT32 m_DirectionalLightCount;
    UINT32 m_PointLightCount;
    UINT32 m_SpotLightCount;
    
    UINT32 m_SpotShadowCount;  // Tracks next available spot shadow slot
    UINT32 m_PointShadowCount; // Tracks next available point shadow slot
    
    FrameConstants m_Constants;
    float m_ClearColor[4];
    UINT32 m_ViewportWidth;
    UINT32 m_ViewportHeight;
    SkySettings m_SkySettings;

public:
    FramePacketBuilder()
        : m_DrawCommandCount(0)
        , m_InstanceCount(0)
        , m_ShadowDrawCommandCount(0)
        , m_ShadowInstanceCount(0)
        , m_MaterialCount(0)
        , m_LightCount(0)
        , m_DirectionalLightCount(0)
        , m_PointLightCount(0)
        , m_SpotLightCount(0)
        , m_SpotShadowCount(0)
        , m_PointShadowCount(0)
        , m_ViewportWidth(0)
        , m_ViewportHeight(0)
    {
        m_DrawCommands.reserve(1024);
        m_InstanceData.reserve(4096);
        m_ShadowDrawCommands.reserve(1024);
        m_ShadowInstanceData.reserve(4096);
        m_Materials.reserve(64);
        m_MaterialHandles.reserve(64);
        m_Lights.reserve(MAX_LIGHTS);
        
        m_ClearColor[0] = 0.1f;
        m_ClearColor[1] = 0.1f;
        m_ClearColor[2] = 0.15f;
        m_ClearColor[3] = 1.0f;
    }
    
    void reset()
    {
        m_DrawCommands.clear();
        m_InstanceData.clear();
        m_ShadowDrawCommands.clear();
        m_ShadowInstanceData.clear();
        m_Materials.clear();
        m_MaterialHandles.clear();
        m_Lights.clear();
        m_DrawCommandCount = 0;
        m_InstanceCount = 0;
        m_ShadowDrawCommandCount = 0;
        m_ShadowInstanceCount = 0;
        m_MaterialCount = 0;
        m_LightCount = 0;
        m_DirectionalLightCount = 0;
        m_PointLightCount = 0;
        m_SpotLightCount = 0;
        m_SpotShadowCount = 0;
        m_PointShadowCount = 0;
    }
    
    void setFrameConstants(const FrameConstants& constants) { m_Constants = constants; }
    void setClearColor(float r, float g, float b, float a)
    {
        m_ClearColor[0] = r;
        m_ClearColor[1] = g;
        m_ClearColor[2] = b;
        m_ClearColor[3] = a;
    }
    void setViewport(UINT32 width, UINT32 height)
    {
        m_ViewportWidth = width;
        m_ViewportHeight = height;
    }
    void setSkySettings(const SkySettings& settings) { m_SkySettings = settings; }
    
    bool addDrawCommand(const DrawCommand& cmd)
    {
        if (m_DrawCommandCount >= MAX_DRAW_COMMANDS) return false;
        m_DrawCommands.push_back(cmd);
        m_DrawCommandCount++;
        return true;
    }
    
    UINT32 addInstances(const PerInstanceData* data, UINT32 count)
    {
        if (m_InstanceCount + count > MAX_INSTANCES) return UINT32_MAX;
        UINT32 startIndex = m_InstanceCount;
        for (UINT32 i = 0; i < count; ++i)
        {
            m_InstanceData.push_back(data[i]);
        }
        m_InstanceCount += count;
        return startIndex;
    }
    
    bool addShadowDrawCommand(const DrawCommand& cmd)
    {
        if (m_ShadowDrawCommandCount >= MAX_DRAW_COMMANDS) return false;
        m_ShadowDrawCommands.push_back(cmd);
        m_ShadowDrawCommandCount++;
        return true;
    }
    
    UINT32 addShadowInstances(const PerInstanceData* data, UINT32 count)
    {
        if (m_ShadowInstanceCount + count > MAX_INSTANCES) return UINT32_MAX;
        UINT32 startIndex = m_ShadowInstanceCount;
        for (UINT32 i = 0; i < count; ++i)
        {
            m_ShadowInstanceData.push_back(data[i]);
        }
        m_ShadowInstanceCount += count;
        return startIndex;
    }
    
    bool addMaterial(hMaterial handle, const MaterialData& data)
    {
        if (m_MaterialCount >= MAX_MATERIALS) return false;
        m_MaterialHandles.push_back(handle);
        m_Materials.push_back(data);
        m_MaterialCount++;
        return true;
    }
    
    bool addLight(const GPULightData& light)
    {
        if (m_LightCount >= MAX_LIGHTS) return false;
        m_Lights.push_back(light);
        m_LightCount++;
        return true;
    }
    
    bool addDirectionalLight(const DirectionalLight& light, const Camera& camera)
    {
        if (m_DirectionalLightCount >= MAX_DIRECTIONAL_LIGHTS) return false;

        GPULightData gpu = light.toGPU();
        
        // Compute CSM matrices if light casts shadows
        if (light.flags & static_cast<UINT32>(LightFlags::LIGHT_CAST_SHADOWS))
        {
            CSMData csm = computeCSM(camera, light.direction.Normalized());
            gpu.cascadeSplits = csm.splitDistances;
            for (UINT32 i = 0; i < DIRECTIONAL_CASCADE_COUNT; ++i)
            {
                gpu.cascadeMatrices[i] = csm.cascades[i].viewProjMatrix;
            }
        }
        
        if (addLight(gpu))
        {
            m_DirectionalLightCount++;
            return true;
        }
        return false;
    }
    
    bool addPointLight(const PointLight& light)
    {
        if (m_PointLightCount >= MAX_POINT_LIGHTS) return false;

        GPULightData gpu = light.toGPU();

        // Assign shadow slots if light casts shadows
        // Point lights need 6 slots for cube map faces
        // shadowIndex stores the BASE slot index (first of 6 consecutive slots)
        if ((light.flags & static_cast<UINT32>(LightFlags::LIGHT_CAST_SHADOWS)) && 
            m_PointShadowCount < MAX_POINT_SHADOW_LIGHTS)
        {
            // Each point light uses 6 slots, stored after spot lights
            // Spot lights use slots 0-15 (MAX_SPOT_SHADOW_LIGHTS)
            // Point lights start at slot 16 onwards
            UINT32 baseSlot = MAX_SPOT_SHADOW_LIGHTS + (m_PointShadowCount * POINT_SHADOW_FACE_COUNT);
            gpu.shadowIndex = static_cast<int>(baseSlot);
            
            // Compute 6 cube face matrices
            computePointShadowMatrices(light, gpu.pointShadowMatrices);
            
            m_PointShadowCount++;
        }
        else
        {
            gpu.shadowIndex = -1;
        }

        if (addLight(gpu))
        {
            m_PointLightCount++;
            return true;
        }
        return false;
    }
    
    bool addSpotLight(const SpotLight& light)
    {
        if (m_SpotLightCount >= MAX_SPOT_LIGHTS) return false;

        GPULightData gpu = light.toGPU();
        
        // Assign shadow slot if light casts shadows
        if ((light.flags & static_cast<UINT32>(LightFlags::LIGHT_CAST_SHADOWS)) && 
            m_SpotShadowCount < MAX_SPOT_SHADOW_LIGHTS)
        {
            gpu.shadowIndex = static_cast<int>(m_SpotShadowCount);
            gpu.spotShadowMatrix = computeSpotShadowMatrix(light);
            m_SpotShadowCount++;
        }
        else
        {
            gpu.shadowIndex = -1;
        }
        
        if (addLight(gpu))
        {
            m_SpotLightCount++;
            return true;
        }
        return false;
    }
    
    FramePacket build()
    {
        FramePacket packet = {};
        
        packet.constants = m_Constants;
        packet.constants.activeLightCount = m_LightCount;
        packet.constants.shadowAtlasSize = static_cast<UINT32>(DIRECTIONAL_SHADOW_ATLAS_SIZE);
        packet.constants.localShadowAtlasSize = static_cast<UINT32>(LOCAL_LIGHT_SHADOW_ATLAS_SIZE);
        
        packet.clearColor[0] = m_ClearColor[0];
        packet.clearColor[1] = m_ClearColor[1];
        packet.clearColor[2] = m_ClearColor[2];
        packet.clearColor[3] = m_ClearColor[3];
        
        packet.drawCommands = m_DrawCommands.data();
        packet.drawCommandCount = m_DrawCommandCount;
        
        packet.shadowDrawCommands = m_ShadowDrawCommands.data();
        packet.shadowDrawCommandCount = m_ShadowDrawCommandCount;
        
        packet.instanceData = m_InstanceData.data();
        packet.instanceDataCount = m_InstanceCount;
        
        packet.shadowInstanceData = m_ShadowInstanceData.data();
        packet.shadowInstanceDataCount = m_ShadowInstanceCount;
        
        packet.materials = m_Materials.data();
        packet.materialHandles = m_MaterialHandles.data();
        packet.materialCount = m_MaterialCount;
        
        packet.lights = m_Lights.data();
        packet.lightCount = m_LightCount;
        
        packet.viewportWidth = m_ViewportWidth;
        packet.viewportHeight = m_ViewportHeight;
        packet.skySettings = m_SkySettings;
        
        return packet;
    }
    
    UINT32 getRemainingDrawCommands() const { return MAX_DRAW_COMMANDS - m_DrawCommandCount; }
    UINT32 getRemainingInstances() const { return MAX_INSTANCES - m_InstanceCount; }
    UINT32 getCurrentInstanceCount() const { return m_InstanceCount; }
    UINT32 getCurrentLightCount() const { return m_LightCount; }
    UINT32 getRemainingLights() const { return MAX_LIGHTS - m_LightCount; }
};
