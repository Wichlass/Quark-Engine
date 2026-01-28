#pragma once

#include <d3d11.h>
#include "../../material.h"
#include "../../framepacket.h"
#include "../../sky.h"
#include "../../../../headeronly/mathematics.h"

// ==================== OBJECT CONSTANT BUFFER ====================
// Per-object GPU data (D3D11 specific)
struct ObjectConstantBuffer
{
    Quark::Mat4 world;
    Quark::Mat4 worldInvTranspose;
    Quark::Mat4 worldViewProjection;
    MaterialData material;
};

// ==================== D3D11 PIPELINE STATE MANAGER ====================
class RSD3D11PipelineManager
{
private:
    ID3D11Device* m_pDevice = nullptr;
    ID3D11DeviceContext* m_pContext = nullptr;
    
    // Rasterizer states
    ID3D11RasterizerState* m_pDefaultRasterizer = nullptr;
    ID3D11RasterizerState* m_pCullFrontRasterizer = nullptr;
    ID3D11RasterizerState* m_pCullNoneRasterizer = nullptr;
    ID3D11RasterizerState* m_pShadowRasterizer = nullptr; // Depth Bias
    
    // Depth stencil states
    ID3D11DepthStencilState* m_pDefaultDepthStencil = nullptr;
    ID3D11DepthStencilState* m_pSkyDepthStencil = nullptr; // Depth test LESS_EQUAL, write OFF
    
    // Blend states
    ID3D11BlendState* m_pOpaqueBlend = nullptr;
    ID3D11BlendState* m_pAlphaBlend = nullptr;
    
    // Samplers
    ID3D11SamplerState* m_pLinearSampler = nullptr;
    ID3D11SamplerState* m_pShadowSampler = nullptr; // Comparison Sampler
    
    // Constant buffers
    ID3D11Buffer* m_pFrameConstantBuffer = nullptr;
    ID3D11Buffer* m_pObjectConstantBuffer = nullptr;
    ID3D11Buffer* m_pSkyConstantBuffer = nullptr;

    // State cache
    ID3D11RasterizerState* m_pCurrentRasterizerState = nullptr;
    ID3D11DepthStencilState* m_pCurrentDepthStencilState = nullptr;
    ID3D11BlendState* m_pCurrentBlendState = nullptr;

private:
    bool createRasterizerStates();
    bool createDepthStencilStates();
    bool createBlendStates();
    bool createSamplers();
    bool createConstantBuffers();
    
public:
    RSD3D11PipelineManager() = default;
    ~RSD3D11PipelineManager();

    bool initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    void shutdown();
    
    void resetStateCache();
    
    // State setters
    void setRasterizerState(CullMode cullMode);
    void setShadowRasterizer();
    void setDefaultDepthStencilState();
    void setSkyDepthStencilState();
    void setOpaqueBlendState();
    void setAlphaBlendState();
    void bindSamplers();
    
    // Constant buffer updates
    void updateFrameConstants(const FrameConstants& data);
    void updateObjectConstants(const ObjectConstantBuffer& data);
    void updateSkyConstants(const SkySettings& settings, float time);
    
    // Constant buffer binding
    void bindFrameConstants();
    void bindObjectConstants();
    void bindSkyConstants();
    
    // Getters
    ID3D11Buffer* getFrameConstantBuffer() const { return m_pFrameConstantBuffer; }
    ID3D11Buffer* getObjectConstantBuffer() const { return m_pObjectConstantBuffer; }
    ID3D11Buffer* getSkyConstantBuffer() const { return m_pSkyConstantBuffer; }
};