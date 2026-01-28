#include "rendersystem.h"
#include <iostream>
#include <cstring>

// ==================== CONSTRUCTOR ====================
RenderSystem::RenderSystem()
    : m_pRhi(nullptr)
    , m_WindowHandle(nullptr)
    , m_ViewportWidth(0)
    , m_ViewportHeight(0)
    , m_pActiveCamera(nullptr)
    , m_DeltaTime(0.016f)
    , m_Time(0.0f)
    , m_NextMeshHandle(1)
    , m_NextMaterialHandle(1)
    , m_NextTextureHandle(1)
    , m_NextLightHandle(1)
    , m_AmbientLight(0.1f, 0.1f, 0.1f, 1.0f)
{
    m_ClearColor[0] = 0.1f;
    m_ClearColor[1] = 0.1f;
    m_ClearColor[2] = 0.15f;
    m_ClearColor[3] = 1.0f;
    
    memset(&m_Stats, 0, sizeof(RenderStats));
    
    std::cout << "[RenderSystem] Created.\n";
}

// ==================== LIFECYCLE ====================
void RenderSystem::init(RHI* rhi, qWndh windowHandle)
{
    if (!rhi)
    {
        std::cerr << "[RenderSystem] ERROR: RHI pointer is null.\n";
        return;
    }

    if (!windowHandle)
    {
        std::cerr << "[RenderSystem] ERROR: Window handle is null.\n";
        return;
    }

    m_pRhi = rhi;
    m_WindowHandle = windowHandle;

    m_pRhi->init(windowHandle);

    std::cout << "[RenderSystem] Initialized successfully.\n";
}

void RenderSystem::shutdown()
{
    std::cout << "[RenderSystem] Shutting down...\n";

    for (auto& pair : m_Meshes)
    {
        if (m_pRhi) m_pRhi->destroyMeshBuffer(pair.second.gpuHandle);
    }
    m_Meshes.clear();

    for (auto& pair : m_Materials)
    {
        if (m_pRhi) m_pRhi->destroyMaterialBuffer(pair.second.gpuHandle);
    }
    m_Materials.clear();

    for (auto& pair : m_Textures)
    {
        if (m_pRhi) m_pRhi->destroyTexture(pair.second);
    }
    m_Textures.clear();

    m_Lights.clear();

    if (m_pRhi)
    {
        m_pRhi->shutdown();
    }

    m_pRhi = nullptr;
    m_pActiveCamera = nullptr;

    std::cout << "[RenderSystem] Shutdown complete.\n";
}

void RenderSystem::renderFrame()
{
    if (!m_pRhi)
    {
        std::cerr << "[RenderSystem] ERROR: Cannot render, RHI is null.\n";
        return;
    }

    if (!m_pActiveCamera)
    {
        std::cerr << "[RenderSystem] WARNING: No active camera set.\n";
        return;
    }

    m_Time += m_DeltaTime;
    
    m_pActiveCamera->update();
    
    memset(&m_Stats, 0, sizeof(RenderStats));
    m_Stats.objectsRendered = 0;
    m_Stats.objectsCulled = 0;

    // ==================== CULLING ====================
    frustumCull();

    // ==================== SORTING ====================
    sortObjects();

    // ==================== BATCHING & BUILD ====================
    buildBatches();
    
    // ==================== BUILD FRAME PACKET ====================
    FramePacket packet = buildFramePacket();

    // ==================== EXECUTE ====================
    m_pRhi->executeFrame(packet);

    // ==================== CLEANUP ====================
    m_SubmittedObjects.clear();
    m_VisibleObjects.clear();
    m_ShadowCasters.clear();
    m_PacketBuilder.reset();

}

void RenderSystem::endFrame()
{
    if (m_pRhi)
    {
        m_pRhi->endFrame();
    }
}

// ==================== CULLING ====================
void RenderSystem::frustumCull()
{
    m_VisibleObjects.clear();
    m_ShadowCasters.clear();
    m_VisibleObjects.reserve(m_SubmittedObjects.size());
    m_ShadowCasters.reserve(m_SubmittedObjects.size());

    for (auto& obj : m_SubmittedObjects)
    {
        if (obj.flags == RenderObjectFlags::NONE)
        {
            continue;
        }
        
        if ((obj.flags & RenderObjectFlags::CAST_SHADOW) != RenderObjectFlags::NONE)
        {
            m_ShadowCasters.push_back(obj);
        }
        
        if ((obj.flags & RenderObjectFlags::VISIBLE) == RenderObjectFlags::NONE)
        {
            continue;
        }
        
        bool shouldCull = (obj.flags & RenderObjectFlags::FRUSTUM_CULL) != RenderObjectFlags::NONE;
        
        if (shouldCull && !m_pActiveCamera->isVisible(obj.worldBounds))
        {
            m_Stats.objectsCulled++;
            continue;
        }
        
        Quark::Vec3 objCenter = obj.worldBounds.Center();
        obj.sortDistance = (objCenter - m_pActiveCamera->position).LengthSq();
        
        m_VisibleObjects.push_back(obj);
        m_Stats.objectsRendered++;
    }
}

// ==================== SORTING ====================
void RenderSystem::sortObjects()
{
    auto isTransparent = [this](hMaterial matHandle) -> bool
    {
        auto it = m_Materials.find(matHandle);
        if (it == m_Materials.end()) return false;
        return (static_cast<MaterialFlags>(it->second.data.flags) & MaterialFlags::ALPHA_BLEND) != MaterialFlags::NONE;
    };

    for (auto& obj : m_VisibleObjects)
    {
        obj.sortKey = calculateSortKey(obj.material, obj.sortDistance);
    }

    std::sort(m_VisibleObjects.begin(), m_VisibleObjects.end(),
        [&isTransparent](const SubmittedObject& a, const SubmittedObject& b)
        {
            bool aTransparent = isTransparent(a.material);
            bool bTransparent = isTransparent(b.material);
            
            if (aTransparent != bTransparent)
                return !aTransparent;
            
            if (aTransparent)
            {
                return a.sortDistance > b.sortDistance;
            }
            else
            {
                if (a.material != b.material)
                    return a.material < b.material;
                
                if (a.mesh != b.mesh)
                    return a.mesh < b.mesh;
                
                if (a.material != b.material)
                    return a.material < b.material;
                
                if (a.mesh != b.mesh)
                    return a.mesh < b.mesh;
                
                return a.sortDistance < b.sortDistance;
            }
        });
}

// ==================== BATCHING ====================
void RenderSystem::buildBatches()
{
    if (m_VisibleObjects.empty()) return;

    hMesh currentMesh = 0;
    hMaterial currentMaterial = 0;
    std::vector<PerInstanceData> currentInstances;
    
    auto flushBatch = [&]()
    {
        if (currentInstances.empty()) return;
        
        auto meshIt = m_Meshes.find(currentMesh);
        auto matIt = m_Materials.find(currentMaterial);
        
        if (meshIt == m_Meshes.end() || matIt == m_Materials.end())
        {
            currentInstances.clear();
            return;
        }
        
        UINT32 instanceStart = m_PacketBuilder.addInstances(
            currentInstances.data(), 
            static_cast<UINT32>(currentInstances.size())
        );
        
        if (instanceStart != UINT32_MAX)
        {
            DrawCommand cmd = {};
            cmd.mesh = meshIt->second.gpuHandle;
            cmd.material = matIt->second.gpuHandle;
            cmd.instanceStart = instanceStart;
            cmd.instanceCount = static_cast<UINT32>(currentInstances.size());
            cmd.sortKey = 0;
            
            m_PacketBuilder.addDrawCommand(cmd);
            m_Stats.drawCalls++;
        }
        
        currentInstances.clear();
    };

    for (const auto& obj : m_VisibleObjects)
    {
        if (obj.mesh != currentMesh || obj.material != currentMaterial)
        {
            flushBatch();
            currentMesh = obj.mesh;
            currentMaterial = obj.material;
        }
        
        PerInstanceData instance = {};
        instance.worldMatrix = obj.worldMatrix;
        instance.worldInvTranspose = obj.worldMatrix.Inverted();
        instance.customData = Quark::Vec4(static_cast<float>(static_cast<UINT32>(obj.flags)), 0, 0, 0);
        
        currentInstances.push_back(instance);
        
        auto meshIt = m_Meshes.find(obj.mesh);
        if (meshIt != m_Meshes.end())
        {
            m_Stats.trianglesRendered += meshIt->second.data.indexCount / 3;
        }
    }
    
    flushBatch();
    
    hMesh shadowCurrentMesh = 0;
    hMaterial shadowCurrentMaterial = 0;
    std::vector<PerInstanceData> shadowCurrentInstances;
    shadowCurrentInstances.reserve(64);
    
    auto flushShadowBatch = [&]()
    {
        if (shadowCurrentInstances.empty() || shadowCurrentMesh == 0) return;
        
        UINT32 instanceStart = m_PacketBuilder.addShadowInstances(shadowCurrentInstances.data(), 
                                                                   static_cast<UINT32>(shadowCurrentInstances.size()));
        if (instanceStart != UINT32_MAX)
        {
            DrawCommand cmd = {};
            
            auto meshIt = m_Meshes.find(shadowCurrentMesh);
            auto matIt = m_Materials.find(shadowCurrentMaterial);
            
            cmd.mesh = (meshIt != m_Meshes.end()) ? meshIt->second.gpuHandle : 0;
            cmd.material = (matIt != m_Materials.end()) ? matIt->second.gpuHandle : 0;
            cmd.instanceStart = instanceStart;
            cmd.instanceCount = static_cast<UINT32>(shadowCurrentInstances.size());
            cmd.sortKey = 0;
            
            m_PacketBuilder.addShadowDrawCommand(cmd);
        }
        
        shadowCurrentInstances.clear();
    };

    for (const auto& obj : m_ShadowCasters)
    {
        if (obj.mesh != shadowCurrentMesh || obj.material != shadowCurrentMaterial)
        {
            flushShadowBatch();
            shadowCurrentMesh = obj.mesh;
            shadowCurrentMaterial = obj.material;
        }
        
        PerInstanceData instance = {};
        instance.worldMatrix = obj.worldMatrix;
        instance.worldInvTranspose = obj.worldMatrix.Inverted();
        instance.customData = Quark::Vec4(static_cast<float>(static_cast<UINT32>(obj.flags)), 0, 0, 0);
        
        shadowCurrentInstances.push_back(instance);
    }
    
    flushShadowBatch();
}


// ==================== BUILD FRAME PACKET ====================
FramePacket RenderSystem::buildFramePacket()
{
    FrameConstants constants = {};
    constants.view = m_pActiveCamera->view;
    constants.projection = m_pActiveCamera->projection;
    constants.viewProjection = m_pActiveCamera->viewProjection;
    constants.invView = m_pActiveCamera->invView;
    constants.invProjection = m_pActiveCamera->invProjection;
    constants.cameraPosition = m_pActiveCamera->position;
    constants.time = m_Time;
    constants.ambientLight = m_AmbientLight;
    constants.deltaTime = m_DeltaTime;
    
    m_PacketBuilder.setFrameConstants(constants);
    m_PacketBuilder.setClearColor(m_ClearColor[0], m_ClearColor[1], m_ClearColor[2], m_ClearColor[3]);
    m_PacketBuilder.setViewport(m_ViewportWidth, m_ViewportHeight);
    
    std::unordered_map<hMaterial, bool> usedMaterials;
    for (const auto& obj : m_VisibleObjects)
    {
        if (usedMaterials.find(obj.material) == usedMaterials.end())
        {
            auto it = m_Materials.find(obj.material);
            if (it != m_Materials.end())
            {
                m_PacketBuilder.addMaterial(it->first, it->second.data);
                usedMaterials[obj.material] = true;
            }
        }
    }
    
    // Sync SkySettings with active directional light
    SkySettings skyForFrame = m_SkySettings;
    
    for (const auto& pair : m_Lights)
    {
        if (!pair.second.isActive) continue;
        
        switch (pair.second.type)
        {
        case LightType::DIRECTIONAL:
            {
                // Sync sun direction and intensity from directional light
                skyForFrame.sunDirection = pair.second.directional.direction.Normalized();
                skyForFrame.sunIntensity = pair.second.directional.intensity;
                
                // Add light to packet
                m_PacketBuilder.addDirectionalLight(pair.second.directional, *m_pActiveCamera);
            }
            break;
        case LightType::POINT:
            m_PacketBuilder.addPointLight(pair.second.point);
            break;
        case LightType::SPOT:
            m_PacketBuilder.addSpotLight(pair.second.spot);
            break;
        }
    }
    
    // Set sky settings with synced sun data
    m_PacketBuilder.setSkySettings(skyForFrame);
    
    return m_PacketBuilder.build();
}

// ==================== UTILITY ====================
UINT32 RenderSystem::calculateSortKey(hMaterial material, float distance)
{
    UINT32 matKey = (material & 0xFFFF) << 16;
    UINT32 distKey = static_cast<UINT32>(distance * 10.0f) & 0xFFFF;
    return matKey | distKey;
}

// ==================== MESH MANAGEMENT ====================
hMesh RenderSystem::createMesh(const MeshData& meshData, bool isDynamic)
{
    if (!m_pRhi)
    {
        std::cerr << "[RenderSystem] ERROR: Cannot create mesh, RHI is null.\n";
        return 0;
    }

    if (!meshData.vertices || meshData.vertexCount == 0)
    {
        std::cerr << "[RenderSystem] ERROR: Invalid mesh data provided.\n";
        return 0;
    }

    hMesh gpuHandle = m_pRhi->createMeshBuffer(meshData, isDynamic);
    if (gpuHandle == 0)
    {
        std::cerr << "[RenderSystem] ERROR: Failed to create GPU mesh buffer.\n";
        return 0;
    }
    
    hMesh localHandle = m_NextMeshHandle++;
    
    MeshResource resource = {};
    resource.data = meshData;
    resource.gpuHandle = gpuHandle;
    resource.isDynamic = isDynamic;
    resource.localBounds = meshData.boundingBox;
    
    m_Meshes[localHandle] = resource;
    
    return localHandle;
}

void RenderSystem::destroyMesh(hMesh handle)
{
    auto it = m_Meshes.find(handle);
    if (it == m_Meshes.end())
    {
        std::cerr << "[RenderSystem] WARNING: Mesh handle not found.\n";
        return;
    }
    
    if (m_pRhi)
    {
        m_pRhi->destroyMeshBuffer(it->second.gpuHandle);
    }
    
    m_Meshes.erase(it);
}

bool RenderSystem::updateMesh(hMesh handle, const MeshData& meshData)
{
    auto it = m_Meshes.find(handle);
    if (it == m_Meshes.end())
    {
        std::cerr << "[RenderSystem] ERROR: Mesh handle not found.\n";
        return false;
    }
    
    if (!it->second.isDynamic)
    {
        std::cerr << "[RenderSystem] ERROR: Cannot update static mesh.\n";
        return false;
    }
    
    if (m_pRhi && m_pRhi->updateMeshBuffer(it->second.gpuHandle, meshData))
    {
        it->second.data = meshData;
        it->second.localBounds = meshData.boundingBox;
        return true;
    }
    
    return false;
}

// ==================== MATERIAL MANAGEMENT ====================
hMaterial RenderSystem::createMaterial(const MaterialData& materialData)
{
    if (!m_pRhi)
    {
        std::cerr << "[RenderSystem] ERROR: Cannot create material, RHI is null.\n";
        return 0;
    }

    hMaterial gpuHandle = m_pRhi->createMaterialBuffer(materialData);
    if (gpuHandle == 0)
    {
        std::cerr << "[RenderSystem] ERROR: Failed to create GPU material buffer.\n";
        return 0;
    }
    
    hMaterial localHandle = m_NextMaterialHandle++;
    
    MaterialResource resource = {};
    resource.data = materialData;
    resource.gpuHandle = gpuHandle;
    
    m_Materials[localHandle] = resource;
    
    return localHandle;
}

void RenderSystem::destroyMaterial(hMaterial handle)
{
    auto it = m_Materials.find(handle);
    if (it == m_Materials.end())
    {
        std::cerr << "[RenderSystem] WARNING: Material handle not found.\n";
        return;
    }
    
    if (m_pRhi)
    {
        m_pRhi->destroyMaterialBuffer(it->second.gpuHandle);
    }
    
    m_Materials.erase(it);
}

bool RenderSystem::updateMaterial(hMaterial handle, const MaterialData& materialData)
{
    auto it = m_Materials.find(handle);
    if (it == m_Materials.end())
    {
        std::cerr << "[RenderSystem] ERROR: Material handle not found.\n";
        return false;
    }
    
    if (m_pRhi && m_pRhi->updateMaterialBuffer(it->second.gpuHandle, materialData))
    {
        it->second.data = materialData;
        return true;
    }
    
    return false;
}

// ==================== TEXTURE MANAGEMENT ====================
hTexture RenderSystem::loadTexture(const char* filename)
{
    if (!m_pRhi)
    {
        std::cerr << "[RenderSystem] ERROR: Cannot load texture, RHI is null.\n";
        return 0;
    }
    
    hTexture gpuHandle = m_pRhi->loadTexture(filename);
    if (gpuHandle == 0)
    {
        std::cerr << "[RenderSystem] ERROR: Failed to load texture: " << filename << "\n";
        return 0;
    }
    
    hTexture localHandle = m_NextTextureHandle++;
    m_Textures[localHandle] = gpuHandle;
    
    return localHandle;
}

void RenderSystem::destroyTexture(hTexture handle)
{
    auto it = m_Textures.find(handle);
    if (it == m_Textures.end())
    {
        std::cerr << "[RenderSystem] WARNING: Texture handle not found.\n";
        return;
    }
    
    if (m_pRhi)
    {
        m_pRhi->destroyTexture(it->second);
    }
    
    m_Textures.erase(it);
}

bool RenderSystem::setMaterialTexture(hMaterial material, hTexture texture, UINT32 slot)
{
    auto matIt = m_Materials.find(material);
    if (matIt == m_Materials.end())
    {
        std::cerr << "[RenderSystem] ERROR: Material handle not found.\n";
        return false;
    }
    
    auto texIt = m_Textures.find(texture);
    if (texIt == m_Textures.end())
    {
        std::cerr << "[RenderSystem] ERROR: Texture handle not found.\n";
        return false;
    }
    
    if (m_pRhi)
    {
        return m_pRhi->bindTextureToMaterial(matIt->second.gpuHandle, texIt->second, slot);
    }
    
    return false;
}

// ==================== OBJECT SUBMISSION ====================
void RenderSystem::submit(const RenderObject& obj)
{
    SubmittedObject submitted = {};
    submitted.mesh = obj.mesh;
    submitted.material = obj.material;
    submitted.worldMatrix = obj.worldMatrix;
    submitted.worldBounds = obj.worldAABB;
    submitted.flags = obj.flags;
    submitted.sortDistance = 0.0f;
    submitted.sortKey = 0;
    
    m_SubmittedObjects.push_back(submitted);
}

// ==================== CAMERA ====================
void RenderSystem::setActiveCamera(Camera* camera)
{
    m_pActiveCamera = camera;
}

Camera* RenderSystem::getActiveCamera() const
{
    return m_pActiveCamera;
}

// ==================== SETTINGS ====================
void RenderSystem::setClearColor(float r, float g, float b, float a)
{
    m_ClearColor[0] = r;
    m_ClearColor[1] = g;
    m_ClearColor[2] = b;
    m_ClearColor[3] = a;
}

void RenderSystem::setAmbientLight(const Quark::Color& color)
{
    m_AmbientLight = color;
}

void RenderSystem::setSkySettings(const SkySettings& settings)
{
    m_SkySettings = settings;
}

const SkySettings& RenderSystem::getSkySettings() const
{
    return m_SkySettings;
}

// ==================== LIGHTING ====================
hLight RenderSystem::createDirectionalLight(const DirectionalLight& data)
{
    UINT32 count = 0;
    for (const auto& pair : m_Lights)
    {
        if (pair.second.type == LightType::DIRECTIONAL) count++;
    }

    if (count >= MAX_DIRECTIONAL_LIGHTS)
    {
        std::cerr << "[RenderSystem] ERROR: Cannot create Directional Light. Limit reached (" << MAX_DIRECTIONAL_LIGHTS << ").\n";
        return 0;
    }

    hLight handle = m_NextLightHandle++;
    
    LightResource resource = {};
    resource.type = LightType::DIRECTIONAL;
    resource.directional = data;
    resource.isActive = true;
    
    m_Lights[handle] = resource;
    return handle;
}

hLight RenderSystem::createPointLight(const PointLight& data)
{
    UINT32 count = 0;
    for (const auto& pair : m_Lights)
    {
        if (pair.second.type == LightType::POINT) count++;
    }

    if (count >= MAX_POINT_LIGHTS)
    {
        std::cerr << "[RenderSystem] ERROR: Cannot create Point Light. Limit reached (" << MAX_POINT_LIGHTS << ").\n";
        return 0;
    }

    hLight handle = m_NextLightHandle++;
    
    LightResource resource = {};
    resource.type = LightType::POINT;
    resource.point = data;
    resource.isActive = true;
    
    m_Lights[handle] = resource;
    return handle;
}

hLight RenderSystem::createSpotLight(const SpotLight& data)
{
    UINT32 count = 0;
    for (const auto& pair : m_Lights)
    {
        if (pair.second.type == LightType::SPOT) count++;
    }

    if (count >= MAX_SPOT_LIGHTS)
    {
        std::cerr << "[RenderSystem] ERROR: Cannot create Spot Light. Limit reached (" << MAX_SPOT_LIGHTS << ").\n";
        return 0;
    }

    hLight handle = m_NextLightHandle++;
    
    LightResource resource = {};
    resource.type = LightType::SPOT;
    resource.spot = data;
    resource.isActive = true;
    
    m_Lights[handle] = resource;
    return handle;
}

void RenderSystem::updateLight(hLight handle, const DirectionalLight& data)
{
    auto it = m_Lights.find(handle);
    if (it != m_Lights.end() && it->second.type == LightType::DIRECTIONAL)
    {
        it->second.directional = data;
    }
}

void RenderSystem::updateLight(hLight handle, const PointLight& data)
{
    auto it = m_Lights.find(handle);
    if (it != m_Lights.end() && it->second.type == LightType::POINT)
    {
        it->second.point = data;
    }
}

void RenderSystem::updateLight(hLight handle, const SpotLight& data)
{
    auto it = m_Lights.find(handle);
    if (it != m_Lights.end() && it->second.type == LightType::SPOT)
    {
        it->second.spot = data;
    }
}

void RenderSystem::destroyLight(hLight handle)
{
    m_Lights.erase(handle);
}

// ==================== STATISTICS ====================
const RenderStats& RenderSystem::getStats() const
{
    return m_Stats;
}

// ==================== TIMING ====================
void RenderSystem::setDeltaTime(float dt)
{
    if (dt < 0.0f) dt = 0.0f;
    m_DeltaTime = dt;
}

float RenderSystem::getDeltaTime() const
{
    return m_DeltaTime;
}

void RenderSystem::setTime(float time)
{
    m_Time = time;
}

float RenderSystem::getTime() const
{
    return m_Time;
}

// ==================== WINDOW ====================
void RenderSystem::onResize(UINT32 width, UINT32 height)
{
    if (width == 0 || height == 0)
    {
        std::cerr << "[RenderSystem] WARNING: Invalid resize dimensions.\n";
        return;
    }
    
    m_ViewportWidth = width;
    m_ViewportHeight = height;
    
    if (m_pRhi)
    {
        m_pRhi->onResize(width, height);
    }
    
    if (m_pActiveCamera)
    {
        float aspect = static_cast<float>(width) / static_cast<float>(height);
        m_pActiveCamera->setAspect(aspect);
    }
}

// ==================== C API ====================
RenderSystemAPI* createRenderSystem()
{
    return new RenderSystem();
}

void destroyRenderSystem(RenderSystemAPI* renderSystem)
{
    if (renderSystem)
    {
        delete renderSystem;
    }
}