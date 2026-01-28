#include "rsd3d11_pipeline.h"
#include <iostream>

// ==================== DESTRUCTOR ====================
RSD3D11PipelineManager::~RSD3D11PipelineManager()
{
    shutdown();
}

// ==================== INITIALIZATION ====================
bool RSD3D11PipelineManager::initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
    m_pDevice = device;
    m_pContext = context;
    
    std::cout << "[RSD3D11PipelineManager] Initializing...\n";
    
    if (!createRasterizerStates())
    {
        std::cerr << "[RSD3D11PipelineManager] Failed to create rasterizer states.\n";
        return false;
    }
    
    if (!createDepthStencilStates())
    {
        std::cerr << "[RSD3D11PipelineManager] Failed to create depth stencil states.\n";
        return false;
    }
    
    if (!createBlendStates())
    {
        std::cerr << "[RSD3D11PipelineManager] Failed to create blend states.\n";
        return false;
    }
    
    if (!createSamplers())
    {
        std::cerr << "[RSD3D11PipelineManager] Failed to create samplers.\n";
        return false;
    }
    
    if (!createConstantBuffers())
    {
        std::cerr << "[RSD3D11PipelineManager] Failed to create constant buffers.\n";
        return false;
    }
    
    std::cout << "[RSD3D11PipelineManager] Initialization complete.\n";
    resetStateCache();
    return true;
}

void RSD3D11PipelineManager::resetStateCache()
{
    m_pCurrentRasterizerState = nullptr;
    m_pCurrentDepthStencilState = nullptr;
    m_pCurrentBlendState = nullptr;
}

void RSD3D11PipelineManager::shutdown()
{
    std::cout << "[RSD3D11PipelineManager] Shutting down...\n";
    
    if (m_pSkyConstantBuffer) { m_pSkyConstantBuffer->Release(); m_pSkyConstantBuffer = nullptr; }
    if (m_pObjectConstantBuffer) { m_pObjectConstantBuffer->Release(); m_pObjectConstantBuffer = nullptr; }
    if (m_pFrameConstantBuffer) { m_pFrameConstantBuffer->Release(); m_pFrameConstantBuffer = nullptr; }
    
    if (m_pLinearSampler) { m_pLinearSampler->Release(); m_pLinearSampler = nullptr; }
    
    if (m_pAlphaBlend) { m_pAlphaBlend->Release(); m_pAlphaBlend = nullptr; }
    if (m_pOpaqueBlend) { m_pOpaqueBlend->Release(); m_pOpaqueBlend = nullptr; }
    
    if (m_pSkyDepthStencil) { m_pSkyDepthStencil->Release(); m_pSkyDepthStencil = nullptr; }
    if (m_pDefaultDepthStencil) { m_pDefaultDepthStencil->Release(); m_pDefaultDepthStencil = nullptr; }
    
    if (m_pCullNoneRasterizer) { m_pCullNoneRasterizer->Release(); m_pCullNoneRasterizer = nullptr; }
    if (m_pCullFrontRasterizer) { m_pCullFrontRasterizer->Release(); m_pCullFrontRasterizer = nullptr; }
    if (m_pDefaultRasterizer) { m_pDefaultRasterizer->Release(); m_pDefaultRasterizer = nullptr; }
    if (m_pShadowRasterizer) { m_pShadowRasterizer->Release(); m_pShadowRasterizer = nullptr; }
    if (m_pShadowSampler) { m_pShadowSampler->Release(); m_pShadowSampler = nullptr; }

    std::cout << "[RSD3D11PipelineManager] Shutdown complete.\n";
}

// ==================== RASTERIZER STATES ====================
bool RSD3D11PipelineManager::createRasterizerStates()
{
    D3D11_RASTERIZER_DESC rastDesc = {};
    rastDesc.FillMode = D3D11_FILL_SOLID;
    rastDesc.FrontCounterClockwise = TRUE; // FALSE = Clockwise is Front (matches row-major matrices with D3D11)
    rastDesc.DepthClipEnable = TRUE;
    rastDesc.DepthBias = 0;
    rastDesc.DepthBiasClamp = 0.0f;
    rastDesc.SlopeScaledDepthBias = 0.0f;

    // Default rasterizer - Back-face culling
    rastDesc.CullMode = D3D11_CULL_BACK;
    HRESULT hr = m_pDevice->CreateRasterizerState(&rastDesc, &m_pDefaultRasterizer);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11PipelineManager] Failed to create default rasterizer.\n";
        return false;
    }

    // Front-face culling rasterizer
    rastDesc.CullMode = D3D11_CULL_FRONT;
    hr = m_pDevice->CreateRasterizerState(&rastDesc, &m_pCullFrontRasterizer);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11PipelineManager] Failed to create front-cull rasterizer.\n";
        return false;
    }

    // No culling rasterizer (double-sided)
    rastDesc.CullMode = D3D11_CULL_NONE;
    hr = m_pDevice->CreateRasterizerState(&rastDesc, &m_pCullNoneRasterizer);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11PipelineManager] Failed to create no-cull rasterizer.\n";
        return false;
    }

    rastDesc.CullMode = D3D11_CULL_BACK;
    rastDesc.DepthBias = 1000;
    rastDesc.DepthBiasClamp = 0.1f;
    rastDesc.SlopeScaledDepthBias = 1.5f;
    
    hr = m_pDevice->CreateRasterizerState(&rastDesc, &m_pShadowRasterizer);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11PipelineManager] Failed to create shadow rasterizer.\n";
        return false;
    }

    std::cout << "[RSD3D11PipelineManager] Rasterizer states created.\n";
    return true;
}

// ==================== DEPTH STENCIL STATES ====================
bool RSD3D11PipelineManager::createDepthStencilStates()
{
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    dsDesc.StencilEnable = FALSE;

    HRESULT hr = m_pDevice->CreateDepthStencilState(&dsDesc, &m_pDefaultDepthStencil);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11PipelineManager] Failed to create default depth stencil.\n";
        return false;
    }

    // Sky depth stencil: depth test LESS_EQUAL (render at far plane), no write
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // Don't write depth
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL; // Pass if <= far plane
    
    hr = m_pDevice->CreateDepthStencilState(&dsDesc, &m_pSkyDepthStencil);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11PipelineManager] Failed to create sky depth stencil.\n";
        return false;
    }

    std::cout << "[RSD3D11PipelineManager] Depth stencil states created.\n";
    return true;
}

// ==================== BLEND STATES ====================
bool RSD3D11PipelineManager::createBlendStates()
{
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = FALSE;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    HRESULT hr = m_pDevice->CreateBlendState(&blendDesc, &m_pOpaqueBlend);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11PipelineManager] Failed to create opaque blend.\n";
        return false;
    }

    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

    hr = m_pDevice->CreateBlendState(&blendDesc, &m_pAlphaBlend);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11PipelineManager] Failed to create alpha blend.\n";
        return false;
    }

    std::cout << "[RSD3D11PipelineManager] Blend states created.\n";
    return true;
}

// ==================== SAMPLERS ====================
bool RSD3D11PipelineManager::createSamplers()
{
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    HRESULT hr = m_pDevice->CreateSamplerState(&sampDesc, &m_pLinearSampler);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11PipelineManager] Failed to create linear sampler.\n";
        return false;
    }

    // Shadow Comparison Sampler (PCF)
    sampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT; // Linear PCF
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    sampDesc.BorderColor[0] = 1.0f; // White border = far away = lit
    sampDesc.BorderColor[1] = 1.0f;
    sampDesc.BorderColor[2] = 1.0f;
    sampDesc.BorderColor[3] = 1.0f;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS; // Shadow check: depth < occluder ?

    hr = m_pDevice->CreateSamplerState(&sampDesc, &m_pShadowSampler);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11PipelineManager] Failed to create shadow sampler.\n";
        return false;
    }

    std::cout << "[RSD3D11PipelineManager] Samplers created.\n";
    return true;
}

// ==================== CONSTANT BUFFERS ====================
bool RSD3D11PipelineManager::createConstantBuffers()
{
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;

    bd.ByteWidth = sizeof(FrameConstants);
    HRESULT hr = m_pDevice->CreateBuffer(&bd, nullptr, &m_pFrameConstantBuffer);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11PipelineManager] Failed to create frame constant buffer.\n";
        return false;
    }

    bd.ByteWidth = sizeof(ObjectConstantBuffer);
    hr = m_pDevice->CreateBuffer(&bd, nullptr, &m_pObjectConstantBuffer);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11PipelineManager] Failed to create object constant buffer.\n";
        return false;
    }

    // Sky constant buffer (512 bytes for volumetric cloud parameters)
    bd.ByteWidth = 512; // Expanded for volumetric cloud system
    hr = m_pDevice->CreateBuffer(&bd, nullptr, &m_pSkyConstantBuffer);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11PipelineManager] Failed to create sky constant buffer.\n";
        return false;
    }

    std::cout << "[RSD3D11PipelineManager] Constant buffers created.\n";
    return true;
}

// ==================== STATE SETTERS ====================
void RSD3D11PipelineManager::setRasterizerState(CullMode cullMode)
{
    if (!m_pContext) return;
    
    ID3D11RasterizerState* targetState = nullptr;
    
    switch (cullMode)
    {
    case CullMode::FRONT:       targetState = m_pCullFrontRasterizer; break;
    case CullMode::DOUBLE_SIDED:
    case CullMode::NONE:        targetState = m_pCullNoneRasterizer; break;
    case CullMode::BACK:
    default:                    targetState = m_pDefaultRasterizer; break;
    }
    
    // State Caching Check
    if (m_pCurrentRasterizerState != targetState)
    {
        m_pContext->RSSetState(targetState);
        m_pCurrentRasterizerState = targetState;
    }
}

void RSD3D11PipelineManager::setDefaultDepthStencilState()
{
    if (m_pContext && m_pDefaultDepthStencil)
    {
        if (m_pCurrentDepthStencilState != m_pDefaultDepthStencil)
        {
            m_pContext->OMSetDepthStencilState(m_pDefaultDepthStencil, 0);
            m_pCurrentDepthStencilState = m_pDefaultDepthStencil;
        }
    }
}

void RSD3D11PipelineManager::setOpaqueBlendState()
{
    if (m_pContext && m_pOpaqueBlend)
    {
        if (m_pCurrentBlendState != m_pOpaqueBlend)
        {
            m_pContext->OMSetBlendState(m_pOpaqueBlend, nullptr, 0xFFFFFFFF);
            m_pCurrentBlendState = m_pOpaqueBlend;
        }
    }
}

void RSD3D11PipelineManager::setAlphaBlendState()
{
    if (m_pContext && m_pAlphaBlend)
    {
        if (m_pCurrentBlendState != m_pAlphaBlend)
        {
            m_pContext->OMSetBlendState(m_pAlphaBlend, nullptr, 0xFFFFFFFF);
            m_pCurrentBlendState = m_pAlphaBlend;
        }
    }
}

void RSD3D11PipelineManager::bindSamplers()
{
    if (m_pContext)
    {
        m_pContext->PSSetSamplers(0, 1, &m_pLinearSampler);
        m_pContext->PSSetSamplers(1, 1, &m_pShadowSampler); // Slot 1: Shadow Sampler (Comparison)
    }
}

// ==================== CONSTANT BUFFER UPDATES ====================
void RSD3D11PipelineManager::updateFrameConstants(const FrameConstants& data)
{
    if (m_pContext && m_pFrameConstantBuffer)
    {
        m_pContext->UpdateSubresource(m_pFrameConstantBuffer, 0, nullptr, &data, 0, 0);
    }
}

void RSD3D11PipelineManager::updateObjectConstants(const ObjectConstantBuffer& data)
{
    if (m_pContext && m_pObjectConstantBuffer)
    {
        m_pContext->UpdateSubresource(m_pObjectConstantBuffer, 0, nullptr, &data, 0, 0);
    }
}

// ==================== CONSTANT BUFFER BINDING ====================
void RSD3D11PipelineManager::bindFrameConstants()
{
    if (m_pContext && m_pFrameConstantBuffer)
    {
        m_pContext->VSSetConstantBuffers(0, 1, &m_pFrameConstantBuffer);
        m_pContext->PSSetConstantBuffers(0, 1, &m_pFrameConstantBuffer);
    }
}

void RSD3D11PipelineManager::bindObjectConstants()
{
    if (m_pContext && m_pObjectConstantBuffer)
    {
        m_pContext->VSSetConstantBuffers(1, 1, &m_pObjectConstantBuffer);
        m_pContext->PSSetConstantBuffers(1, 1, &m_pObjectConstantBuffer);
    }
}
// ==================== SHADOW STATES IMPL ====================
// Note: Call this inside createRasterizerStates or similar
// For simplicity, we assume this logic is integrated or called separately. 
// Since we can't easily inject into the middle of a function with append, 
// we'll rely on the user manually checking or we can use replace for the whole function.
// BUT, the safer way for "append" strategy is to adding new methods.

// We need to Modify createRasterizerStates to include ShadowRasterizer creation.
// And Modify createSamplers to include ShadowSampler.
// And Add setShadowRasterizer.

void RSD3D11PipelineManager::setShadowRasterizer()
{
    if (m_pContext && m_pShadowRasterizer)
    {
        if (m_pCurrentRasterizerState != m_pShadowRasterizer)
        {
            m_pContext->RSSetState(m_pShadowRasterizer);
            m_pCurrentRasterizerState = m_pShadowRasterizer;
        }
    }
}

void RSD3D11PipelineManager::setSkyDepthStencilState()
{
    if (m_pContext && m_pSkyDepthStencil)
    {
        if (m_pCurrentDepthStencilState != m_pSkyDepthStencil)
        {
            m_pContext->OMSetDepthStencilState(m_pSkyDepthStencil, 0);
            m_pCurrentDepthStencilState = m_pSkyDepthStencil;
        }
    }
}

void RSD3D11PipelineManager::updateSkyConstants(const SkySettings& settings, float time)
{
    if (!m_pContext || !m_pSkyConstantBuffer) return;
    
    GPUSkyData gpu = settings.toGPU(time);
    m_pContext->UpdateSubresource(m_pSkyConstantBuffer, 0, nullptr, &gpu, 0, 0);
}

void RSD3D11PipelineManager::bindSkyConstants()
{
    if (m_pContext && m_pSkyConstantBuffer)
    {
        m_pContext->VSSetConstantBuffers(2, 1, &m_pSkyConstantBuffer);
        m_pContext->PSSetConstantBuffers(2, 1, &m_pSkyConstantBuffer);
    }
}
