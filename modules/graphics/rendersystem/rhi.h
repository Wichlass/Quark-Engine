#pragma once

#include "../../headeronly/globaltypes.h"
#include "rstypes.h"
#include "meshdata.h"
#include "material.h"
#include "framepacket.h"

// ==================== RHI (Rendering Hardware Interface) ====================
class RHI
{
public:
    virtual ~RHI() = default;

    // ==================== LIFECYCLE ====================
    virtual void init(qWndh windowHandle) = 0;
    virtual void shutdown() = 0;

    // ==================== GPU MESH BUFFERS ====================
    virtual hMesh createMeshBuffer(const MeshData& meshData, bool isDynamic) = 0;
    virtual void destroyMeshBuffer(hMesh handle) = 0;
    virtual bool updateMeshBuffer(hMesh handle, const MeshData& meshData) = 0;

    // ==================== GPU MATERIAL BUFFERS ====================
    virtual hMaterial createMaterialBuffer(const MaterialData& materialData) = 0;
    virtual void destroyMaterialBuffer(hMaterial handle) = 0;
    virtual bool updateMaterialBuffer(hMaterial handle, const MaterialData& materialData) = 0;

    // ==================== TEXTURES ====================
    virtual hTexture loadTexture(const char* filename) = 0;
    virtual void destroyTexture(hTexture handle) = 0;
    virtual bool bindTextureToMaterial(hMaterial material, hTexture texture, UINT32 slot) = 0;

    // ==================== FRAME EXECUTION ====================
    virtual void executeFrame(const FramePacket& packet) = 0;
    virtual void endFrame() = 0;

    // ==================== WINDOW EVENTS ====================
    virtual void onResize(UINT32 width, UINT32 height) = 0;

    // ==================== DEBUG (Backend-specific) ====================
    virtual void* getDevice() const = 0;
    virtual void* getContext() const = 0;
};
