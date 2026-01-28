#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

// ==================== D3D11 SHADER MANAGER ====================
class RSD3D11ShaderManager
{
private:
    ID3D11Device* m_pDevice = nullptr;
    ID3D11DeviceContext* m_pContext = nullptr;
    
    // PBR Pipeline
    ID3D11VertexShader* m_pPBRVertexShader = nullptr;
    ID3D11PixelShader* m_pPBRPixelShader = nullptr;
    ID3D11InputLayout* m_pPBRInputLayout = nullptr;

    // Shadow Pipeline
    ID3D11VertexShader* m_pShadowVertexShader = nullptr;

    // Sky Pipeline
    ID3D11VertexShader* m_pSkyVertexShader = nullptr;
    ID3D11PixelShader* m_pSkyPixelShader = nullptr;

    bool compileShaderFromSource(const char* source, const char* entryPoint,
        const char* target, ID3DBlob** outBlob, const D3D_SHADER_MACRO* defines = nullptr);
    bool createPBRShaders();
    bool createShadowShaders();
    bool createSkyShaders();

public:
    RSD3D11ShaderManager() = default;
    ~RSD3D11ShaderManager();

    bool initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    void shutdown();
    
    void bindPBRPipeline();
    void bindShadowPipeline();
    void bindSkyPipeline();
    
    // Getters
    ID3D11VertexShader* getPBRVertexShader() const { return m_pPBRVertexShader; }
    ID3D11PixelShader* getPBRPixelShader() const { return m_pPBRPixelShader; }
    ID3D11InputLayout* getPBRInputLayout() const { return m_pPBRInputLayout; }
    ID3D11VertexShader* getShadowVertexShader() const { return m_pShadowVertexShader; }
    ID3D11VertexShader* getSkyVertexShader() const { return m_pSkyVertexShader; }
    ID3D11PixelShader* getSkyPixelShader() const { return m_pSkyPixelShader; }
};