#pragma once

#ifdef _WIN32
#ifdef RENDERSYSTEM_EXPORTS
#define RENDERSYSTEM_API __declspec(dllexport)
#else
#define RENDERSYSTEM_API __declspec(dllimport)
#endif
#else
#define RENDERSYSTEM_API
#endif

#include <vector>
#include <unordered_map>
#include <algorithm>

#include "../../headeronly/globaltypes.h"
#include "../../headeronly/mathematics.h"
#include "rendersystemapi.h"
#include "rhi.h"
#include "rstypes.h"
#include "renderobject.h"
#include "meshdata.h"
#include "material.h"
#include "lighting.h"
#include "renderstats.h"
#include "camera.h"
#include "framepacket.h"

// ==================== INTERNAL RESOURCE STRUCTURES ====================
// Mesh resource - CPU data + GPU handle
struct MeshResource
{
    MeshData data;
    hMesh gpuHandle;
    bool isDynamic;
    Quark::AABB localBounds;
};

// Material resource - CPU data + GPU handle
struct MaterialResource
{
    MaterialData data;
    hMaterial gpuHandle;
};

struct LightResource
{
    LightType type;
    union
    {
        DirectionalLight directional;
        PointLight point;
        SpotLight spot;
    };
    bool isActive;
    
    LightResource() : type(LightType::DIRECTIONAL), isActive(false) 
    {
        memset(&directional, 0, sizeof(DirectionalLight));
    }
    ~LightResource() {}
};

// ==================== SUBMITTED OBJECT ====================
struct SubmittedObject
{
    hMesh mesh;
    hMaterial material;
    Quark::Mat4 worldMatrix;
    Quark::AABB worldBounds;
    RenderObjectFlags flags;
    float sortDistance;
    UINT32 sortKey;
};

// ==================== RENDER SYSTEM ====================
class RenderSystem : public RenderSystemAPI
{
private:
    // Backend
    RHI* m_pRhi;
    
    // Window
    qWndh m_WindowHandle;
    UINT32 m_ViewportWidth;
    UINT32 m_ViewportHeight;
    
    // Camera
    Camera* m_pActiveCamera;
    
    // Timing
    float m_DeltaTime;
    float m_Time;
    
    // Settings
    float m_ClearColor[4];
    Quark::Color m_AmbientLight;
    
    // ==================== RESOURCE STORAGE ====================
    std::unordered_map<hMesh, MeshResource> m_Meshes;
    std::unordered_map<hMaterial, MaterialResource> m_Materials;
    std::unordered_map<hTexture, hTexture> m_Textures;
    std::unordered_map<hLight, LightResource> m_Lights;
    
    hMesh m_NextMeshHandle;
    hMaterial m_NextMaterialHandle;
    hTexture m_NextTextureHandle;
    hLight m_NextLightHandle;
    
    // ==================== RENDER QUEUE ====================
    std::vector<SubmittedObject> m_SubmittedObjects;
    std::vector<SubmittedObject> m_VisibleObjects;
    std::vector<SubmittedObject> m_ShadowCasters;

    // ==================== SKY ====================
    SkySettings m_SkySettings;
    
    // ==================== FRAME BUILDING ====================
    FramePacketBuilder m_PacketBuilder;
    
    // ==================== STATS ====================
    RenderStats m_Stats;
    
private:
    // ==================== INTERNAL METHODS ====================
    void frustumCull();
    void sortObjects();
    void buildBatches();
    FramePacket buildFramePacket();
    
    UINT32 calculateSortKey(hMaterial material, float distance);
    
public:
    RenderSystem();
    ~RenderSystem() override = default;

    // ==================== LIFECYCLE ====================
    void init(RHI* rhi, qWndh windowHandle) override;
    void shutdown() override;
    void renderFrame() override;
    void endFrame() override;

    // ==================== MESH MANAGEMENT ====================
    hMesh createMesh(const MeshData& meshData, bool isDynamic = false) override;
    void destroyMesh(hMesh handle) override;
    bool updateMesh(hMesh handle, const MeshData& meshData) override;

    // ==================== MATERIAL MANAGEMENT ====================
    hMaterial createMaterial(const MaterialData& materialData) override;
    void destroyMaterial(hMaterial handle) override;
    bool updateMaterial(hMaterial handle, const MaterialData& materialData) override;
    
    // ==================== TEXTURE MANAGEMENT ====================
    hTexture loadTexture(const char* filename) override;
    void destroyTexture(hTexture handle) override;
    bool setMaterialTexture(hMaterial material, hTexture texture, UINT32 slot) override;

    // ==================== OBJECT SUBMISSION ====================
    void submit(const RenderObject& obj) override;

    // ==================== CAMERA ====================
    void setActiveCamera(Camera* camera) override;
    Camera* getActiveCamera() const override;

    // ==================== SETTINGS ====================
    void setClearColor(float r, float g, float b, float a) override;
    void setAmbientLight(const Quark::Color& color) override;
    void setSkySettings(const SkySettings& settings) override;
    const SkySettings& getSkySettings() const override;

    // ==================== LIGHTING ====================
    hLight createDirectionalLight(const DirectionalLight& data) override;
    hLight createPointLight(const PointLight& data) override;
    hLight createSpotLight(const SpotLight& data) override;
    void updateLight(hLight handle, const DirectionalLight& data) override;
    void updateLight(hLight handle, const PointLight& data) override;
    void updateLight(hLight handle, const SpotLight& data) override;
    void destroyLight(hLight handle) override;

    // ==================== STATISTICS ====================
    const RenderStats& getStats() const override;

    // ==================== TIMING ====================
    void setDeltaTime(float dt) override;
    float getDeltaTime() const override;
    void setTime(float time) override;
    float getTime() const override;

    // ==================== WINDOW ====================
    void onResize(UINT32 width, UINT32 height) override;
};

// ==================== C API (DLL BOUNDARY) ====================
extern "C" {
    RENDERSYSTEM_API RenderSystemAPI* createRenderSystem();
    RENDERSYSTEM_API void destroyRenderSystem(RenderSystemAPI* renderSystem);
}