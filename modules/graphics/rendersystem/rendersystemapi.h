#pragma once

#include "rhi.h"
#include "rstypes.h"
#include "renderobject.h"
#include "meshdata.h"
#include "material.h"
#include "lighting.h"
#include "renderstats.h"
#include "camera.h"
#include "sky.h"
#include "../../headeronly/globaltypes.h"
#include "../../headeronly/mathematics.h"

// ==================== RENDER SYSTEM API ====================
class RenderSystemAPI
{
public: 
    virtual ~RenderSystemAPI() = default;

    // ==================== LIFECYCLE ====================
    virtual void init(RHI* rhi, qWndh windowHandle) = 0;
    virtual void shutdown() = 0;
    virtual void renderFrame() = 0;
    virtual void endFrame() = 0;

    // ==================== MESH MANAGEMENT ====================
    virtual hMesh createMesh(const MeshData& meshData, bool isDynamic = false) = 0;
    virtual void destroyMesh(hMesh handle) = 0;
    virtual bool updateMesh(hMesh handle, const MeshData& meshData) = 0;

    // ==================== MATERIAL MANAGEMENT ====================
    virtual hMaterial createMaterial(const MaterialData& materialData) = 0;
    virtual void destroyMaterial(hMaterial handle) = 0;
    virtual bool updateMaterial(hMaterial handle, const MaterialData& materialData) = 0;
    
    // ==================== TEXTURE MANAGEMENT ====================
    virtual hTexture loadTexture(const char* filename) = 0;
    virtual void destroyTexture(hTexture handle) = 0;
    virtual bool setMaterialTexture(hMaterial material, hTexture texture, UINT32 slot) = 0;

    // ==================== OBJECT SUBMISSION ====================
    virtual void submit(const RenderObject& obj) = 0;

    // ==================== CAMERA ====================
    virtual void setActiveCamera(Camera* camera) = 0;
    virtual Camera* getActiveCamera() const = 0;

    // ==================== SETTINGS ====================
    virtual void setClearColor(float r, float g, float b, float a) = 0;
    virtual void setAmbientLight(const Quark::Color& color) = 0;
    virtual void setSkySettings(const SkySettings& settings) = 0;
    virtual const SkySettings& getSkySettings() const = 0;

    // ==================== LIGHTING ====================
    virtual hLight createDirectionalLight(const DirectionalLight& data) = 0;
    virtual hLight createPointLight(const PointLight& data) = 0;
    virtual hLight createSpotLight(const SpotLight& data) = 0;
    virtual void updateLight(hLight handle, const DirectionalLight& data) = 0;
    virtual void updateLight(hLight handle, const PointLight& data) = 0;
    virtual void updateLight(hLight handle, const SpotLight& data) = 0;
    virtual void destroyLight(hLight handle) = 0;
    
    // ==================== STATISTICS ====================
    virtual const RenderStats& getStats() const = 0;

    // ==================== TIMING ====================
    virtual void setDeltaTime(float dt) = 0;
    virtual float getDeltaTime() const = 0;
    virtual void setTime(float time) = 0;
    virtual float getTime() const = 0;

    // ==================== WINDOW ====================
    virtual void onResize(UINT32 width, UINT32 height) = 0;
};