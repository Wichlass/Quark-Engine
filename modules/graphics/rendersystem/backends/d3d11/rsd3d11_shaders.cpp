#include "rsd3d11_shaders.h"
#include "shaders/rsd3d11_main_ps.h"
#include "shaders/rsd3d11_main_vs.h"
#include "shaders/rsd3d11_shadow_vs.h"
#include "shaders/rsd3d11_sky_vs.h"
#include "shaders/rsd3d11_sky_ps.h"
#include "../../lighting.h"
#include <iostream>
#include <vector>
#include <string>

// ==================== DESTRUCTOR ====================
RSD3D11ShaderManager::~RSD3D11ShaderManager()
{
    shutdown();
}

// ==================== INITIALIZATION ====================
bool RSD3D11ShaderManager::initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
    m_pDevice = device;
    m_pContext = context;
    
    std::cout << "[RSD3D11ShaderManager] Initializing...\n";
    
    if (!createPBRShaders())
    {
        std::cerr << "[RSD3D11ShaderManager] Failed to create PBR shaders.\n";
        return false;
    }

    if (!createShadowShaders())
    {
        std::cerr << "[RSD3D11ShaderManager] Failed to create Shadow shaders.\n";
        return false;
    }

    if (!createSkyShaders())
    {
        std::cerr << "[RSD3D11ShaderManager] Failed to create Sky shaders.\n";
        return false;
    }

    std::cout << "[RSD3D11ShaderManager] Initialization complete.\n";
    return true;
}

void RSD3D11ShaderManager::shutdown()
{
    std::cout << "[RSD3D11ShaderManager] Shutting down...\n";
    
    if (m_pPBRInputLayout) { m_pPBRInputLayout->Release(); m_pPBRInputLayout = nullptr; }
    if (m_pPBRPixelShader) { m_pPBRPixelShader->Release(); m_pPBRPixelShader = nullptr; }
    if (m_pPBRVertexShader) { m_pPBRVertexShader->Release(); m_pPBRVertexShader = nullptr; }
    if (m_pShadowVertexShader) { m_pShadowVertexShader->Release(); m_pShadowVertexShader = nullptr; }
    if (m_pSkyVertexShader) { m_pSkyVertexShader->Release(); m_pSkyVertexShader = nullptr; }
    if (m_pSkyPixelShader) { m_pSkyPixelShader->Release(); m_pSkyPixelShader = nullptr; }
    
    std::cout << "[RSD3D11ShaderManager] Shutdown complete.\n";
}

// ==================== SHADER COMPILATION ====================
bool RSD3D11ShaderManager::compileShaderFromSource(const char* source, const char* entryPoint,
    const char* target, ID3DBlob** outBlob, const D3D_SHADER_MACRO* defines)
{
    ID3DBlob* errorBlob = nullptr;

    HRESULT hr = D3DCompile(
        source,
        strlen(source),
        nullptr,
        defines,
        nullptr,
        entryPoint,
        target,
        D3DCOMPILE_ENABLE_STRICTNESS,
        0,
        outBlob,
        &errorBlob
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            std::cerr << "[RSD3D11ShaderManager] Shader compilation error: "
                << (char*)errorBlob->GetBufferPointer() << "\n";
            errorBlob->Release();
        }
        return false;
    }

    return true;
}

// ==================== PBR SHADERS ====================
bool RSD3D11ShaderManager::createPBRShaders()
{
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;

    // Compile PBR vertex shader
    if (!compileShaderFromSource(g_PBRVertexShaderSource, "main", "vs_5_0", &vsBlob))
    {
        std::cerr << "[RSD3D11ShaderManager] Failed to compile PBR vertex shader.\n";
        return false;
    }

    HRESULT hr = m_pDevice->CreateVertexShader(
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        nullptr,
        &m_pPBRVertexShader
    );

    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11ShaderManager] Failed to create PBR vertex shader.\n";
        vsBlob->Release();
        return false;
    }

    // Create input layout for PBR with Instancing support
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        // Per-vertex data (Slot 0)
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        
        // Per-instance data (Slot 1)
        // WORLD matrix (4 rows)
        { "WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        
        // INVWORLD matrix (4 rows) - using SemanticName INVWORLD to match HLSL
        { "INVWORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "INVWORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 80, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "INVWORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 96, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "INVWORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 112, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        
        // CUSTOM data
        { "CUSTOM", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 128, D3D11_INPUT_PER_INSTANCE_DATA, 1 }
    };

    hr = m_pDevice->CreateInputLayout(
        layout,
        ARRAYSIZE(layout),
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        &m_pPBRInputLayout
    );

    vsBlob->Release();

    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11ShaderManager] Failed to create PBR input layout.\n";
        return false;
    }

    // Prepare macros from lighting.h
    std::string maxLightsStr = std::to_string(MAX_LIGHTS);
    std::string cascadeCountStr = std::to_string(DIRECTIONAL_CASCADE_COUNT);
    std::string shadowGridSizeStr = std::to_string(LOCAL_SHADOW_GRID_SIZE);
    
    std::string typeNoneStr = std::to_string(static_cast<int>(LightType::NONE));
    std::string typeDirStr = std::to_string(static_cast<int>(LightType::DIRECTIONAL));
    std::string typePointStr = std::to_string(static_cast<int>(LightType::POINT));
    std::string typeSpotStr = std::to_string(static_cast<int>(LightType::SPOT));
    
    std::string flagEnabledStr = std::to_string(static_cast<int>(LightFlags::LIGHT_ENABLED));
    std::string flagShadowStr = std::to_string(static_cast<int>(LightFlags::LIGHT_CAST_SHADOWS));

    D3D_SHADER_MACRO defines[] = {
        { "MAX_LIGHTS", maxLightsStr.c_str() },
        { "DIRECTIONAL_CASCADE_COUNT", cascadeCountStr.c_str() },
        { "LOCAL_SHADOW_GRID_SIZE", shadowGridSizeStr.c_str() },
        { "LIGHT_TYPE_NONE", typeNoneStr.c_str() },
        { "LIGHT_TYPE_DIRECTIONAL", typeDirStr.c_str() },
        { "LIGHT_TYPE_POINT", typePointStr.c_str() },
        { "LIGHT_TYPE_SPOT", typeSpotStr.c_str() },
        { "LIGHT_FLAG_ENABLED", flagEnabledStr.c_str() },
        { "LIGHT_FLAG_CAST_SHADOWS", flagShadowStr.c_str() },
        { nullptr, nullptr }
    };

    // Compile PBR pixel shader with defines
    if (!compileShaderFromSource(g_PBRPixelShaderSource.c_str(), "main", "ps_5_0", &psBlob, defines))
    {
        std::cerr << "[RSD3D11ShaderManager] Failed to compile PBR pixel shader.\n";
        return false;
    }

    hr = m_pDevice->CreatePixelShader(
        psBlob->GetBufferPointer(),
        psBlob->GetBufferSize(),
        nullptr,
        &m_pPBRPixelShader
    );

    psBlob->Release();

    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11ShaderManager] Failed to create PBR pixel shader.\n";
        return false;
    }

    std::cout << "[RSD3D11ShaderManager] PBR shaders created successfully.\n";
    return true;
}

// ==================== PIPELINE BINDING ====================
void RSD3D11ShaderManager::bindPBRPipeline()
{
    if (m_pContext)
    {
        m_pContext->VSSetShader(m_pPBRVertexShader, nullptr, 0);
        m_pContext->PSSetShader(m_pPBRPixelShader, nullptr, 0);
        m_pContext->IASetInputLayout(m_pPBRInputLayout);
    }
}

// ==================== SHADOW SHADER IMPL ====================
bool RSD3D11ShaderManager::createShadowShaders()
{
    ID3DBlob* vsBlob = nullptr;

    // Compile Shadow vertex shader
    if (!compileShaderFromSource(g_ShadowVertexShaderSource, "main", "vs_5_0", &vsBlob))
    {
        std::cerr << "[RSD3D11ShaderManager] Failed to compile Shadow vertex shader.\n";
        return false;
    }

    HRESULT hr = m_pDevice->CreateVertexShader(
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        nullptr,
        &m_pShadowVertexShader
    );

    vsBlob->Release();

    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11ShaderManager] Failed to create Shadow vertex shader.\n";
        return false;
    }

    std::cout << "[RSD3D11ShaderManager] Shadow shaders created successfully.\n";
    return true;
}

void RSD3D11ShaderManager::bindShadowPipeline()
{
    if (m_pContext)
    {
        // Bind Shadow VS
        m_pContext->VSSetShader(m_pShadowVertexShader, nullptr, 0);
        
        // Unbind PS (Depth Only)
        m_pContext->PSSetShader(nullptr, nullptr, 0);
        
        // Reuse PBR Layout (same vertex structure)
        m_pContext->IASetInputLayout(m_pPBRInputLayout);
    }
}

// ==================== SKY SHADER IMPL ====================
bool RSD3D11ShaderManager::createSkyShaders()
{
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;

    // Compile Sky vertex shader
    if (!compileShaderFromSource(g_SkyVertexShaderSource, "main", "vs_5_0", &vsBlob))
    {
        std::cerr << "[RSD3D11ShaderManager] Failed to compile Sky vertex shader.\n";
        return false;
    }

    HRESULT hr = m_pDevice->CreateVertexShader(
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        nullptr,
        &m_pSkyVertexShader
    );

    vsBlob->Release();

    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11ShaderManager] Failed to create Sky vertex shader.\n";
        return false;
    }

    // Compile Sky pixel shader
    if (!compileShaderFromSource(g_SkyPixelShaderSource.c_str(), "main", "ps_5_0", &psBlob))
    {
        std::cerr << "[RSD3D11ShaderManager] Failed to compile Sky pixel shader.\n";
        return false;
    }

    hr = m_pDevice->CreatePixelShader(
        psBlob->GetBufferPointer(),
        psBlob->GetBufferSize(),
        nullptr,
        &m_pSkyPixelShader
    );

    psBlob->Release();

    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11ShaderManager] Failed to create Sky pixel shader.\n";
        return false;
    }

    std::cout << "[RSD3D11ShaderManager] Sky shaders created successfully.\n";
    return true;
}

void RSD3D11ShaderManager::bindSkyPipeline()
{
    if (m_pContext)
    {
        // Bind Sky VS and PS
        m_pContext->VSSetShader(m_pSkyVertexShader, nullptr, 0);
        m_pContext->PSSetShader(m_pSkyPixelShader, nullptr, 0);
        
        // No input layout needed (full-screen triangle uses SV_VertexID)
        m_pContext->IASetInputLayout(nullptr);
    }
}
