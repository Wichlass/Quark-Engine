#pragma once

#ifdef _WIN32
#ifdef RSD3D11_EXPORTS
#define RSD3D11_API __declspec(dllexport)
#else
#define RSD3D11_API __declspec(dllimport)
#endif
#else
#define RSD3D11_API
#endif

#include <Windows.h>
#include <d3d11.h>
#include <memory>
#include <unordered_map>

#include "../../rhi.h"
#include "../../rstypes.h"
#include "../../meshdata.h"
#include "../../material.h"
#include "../../framepacket.h"
#include "../../../../headeronly/globaltypes.h"
#include "../../../../headeronly/mathematics.h"
#include "rsd3d11_device.h"
#include "rsd3d11_memory.h"
#include "rsd3d11_shaders.h"
#include "rsd3d11_pipeline.h"

// ==================== GPU MESH BUFFER ====================
struct D3D11MeshBuffer
{
    ID3D11Buffer* pVertexBuffer;
    ID3D11Buffer* pIndexBuffer;
    UINT32 vertexCount;
    UINT32 indexCount;
    UINT32 vertexStride;
    bool isDynamic;
};

// ==================== GPU MATERIAL BUFFER ====================
struct D3D11MaterialBuffer
{
    ID3D11Buffer* pConstantBuffer;
    MaterialData data;
    ID3D11ShaderResourceView* textures[6];
};

// ==================== GPU TEXTURE ====================
struct D3D11Texture
{
    ID3D11Texture2D* pTexture;
    ID3D11ShaderResourceView* pSRV;
};

// ==================== RSD3D11 BACKEND ====================
class RSD3D11 : public RHI
{
private:
    // Device/Context
    std::unique_ptr<RSD3D11Device> m_pDevice;
    std::unique_ptr<RSD3D11ShaderManager> m_pShaderManager;
    std::unique_ptr<RSD3D11PipelineManager> m_pPipelineManager;
    std::unique_ptr<RSD3D11MemoryManager> m_pMemoryManager;
    
    // GPU Resources
    std::unordered_map<hMesh, D3D11MeshBuffer> m_MeshBuffers;
    std::unordered_map<hMaterial, D3D11MaterialBuffer> m_MaterialBuffers;
    std::unordered_map<hTexture, D3D11Texture> m_Textures;
    
    hMesh m_NextMeshHandle;
    hMaterial m_NextMaterialHandle;
    hTexture m_NextTextureHandle;
    
    // Frame execution buffers
    ID3D11Buffer* m_pFrameConstantBuffer;
    ID3D11Buffer* m_pMaterialConstantBuffer;
    ID3D11Buffer* m_pInstanceBuffer;
    size_t m_InstanceBufferSize;
    
    // Light structured buffer
    ID3D11Buffer* m_pLightBuffer;
    ID3D11ShaderResourceView* m_pLightBufferSRV;
    
    // Sampler
    ID3D11SamplerState* m_pDefaultSampler;
    
    // Sky sphere mesh
    ID3D11Buffer* m_pSkyVertexBuffer = nullptr;
    ID3D11Buffer* m_pSkyIndexBuffer = nullptr;
    UINT32 m_SkyIndexCount = 0;
    
public:
    RSD3D11();
    ~RSD3D11() override;

    // Disable copy/move
    RSD3D11(const RSD3D11&) = delete;
    RSD3D11& operator=(const RSD3D11&) = delete;
    RSD3D11(RSD3D11&&) = delete;
    RSD3D11& operator=(RSD3D11&&) = delete;

    // ==================== RHI INTERFACE ====================
    void init(qWndh windowHandle) override;
    void shutdown() override;

    // GPU Resource Creation
    hMesh createMeshBuffer(const MeshData& meshData, bool isDynamic) override;
    void destroyMeshBuffer(hMesh handle) override;
    bool updateMeshBuffer(hMesh handle, const MeshData& meshData) override;

    hMaterial createMaterialBuffer(const MaterialData& materialData) override;
    void destroyMaterialBuffer(hMaterial handle) override;
    bool updateMaterialBuffer(hMaterial handle, const MaterialData& materialData) override;

    hTexture loadTexture(const char* filename) override;
    void destroyTexture(hTexture handle) override;
    bool bindTextureToMaterial(hMaterial material, hTexture texture, UINT32 slot) override;

    // Frame Execution
    void executeFrame(const FramePacket& packet) override;
    void endFrame() override;

    void onResize(UINT32 width, UINT32 height) override;

    void* getDevice() const override;
    void* getContext() const override;

private:
    // Internal helpers
    void uploadFrameConstants(const FramePacket& packet);
    void uploadInstanceData(const FramePacket& packet);
    void uploadLightData(const FramePacket& packet);
    void executeDrawCommands(const FramePacket& packet);
    void renderShadowPass(const FramePacket& packet);       // Directional CSM
    void renderSpotShadowPass(const FramePacket& packet);   // Spot light shadows
    void renderPointShadowPass(const FramePacket& packet);  // Point light cube map shadows
    void renderSky(const FramePacket& packet);              // Sky
    
    bool createInstanceBuffer(size_t size);
    bool resizeInstanceBufferIfNeeded(size_t requiredSize);
    bool createLightBuffer();
    bool createSkySphere();
};

// ==================== C API (DLL BOUNDARY) ====================
extern "C" {
    RSD3D11_API RHI* createRenderBackend();
    RSD3D11_API void destroyRenderBackend(RHI* rhi);
}