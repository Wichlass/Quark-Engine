#pragma once

#include <d3d11.h>
#include <dxgi.h>
#include <Windows.h>
#include "../../../../headeronly/globaltypes.h"

// ==================== D3D11 DEVICE MANAGEMENT ====================
class RSD3D11Device
{
private:
    ID3D11Device* m_pDevice = nullptr;
    ID3D11DeviceContext* m_pContext = nullptr;
    IDXGISwapChain* m_pSwapChain = nullptr;
    ID3D11RenderTargetView* m_pRenderTargetView = nullptr;
    ID3D11DepthStencilView* m_pDepthStencilView = nullptr;
    ID3D11Texture2D* m_pDepthStencilBuffer = nullptr;
    
    D3D11_VIEWPORT m_Viewport = {};
    UINT32 m_Width = 0;
    UINT32 m_Height = 0;
    
    // Shadow Map Resources (Directional CSM)
    ID3D11Texture2D* m_pShadowAtlasTexture = nullptr;
    ID3D11DepthStencilView* m_pShadowAtlasDSV = nullptr;
    ID3D11ShaderResourceView* m_pShadowAtlasSRV = nullptr;
    D3D11_VIEWPORT m_ShadowViewport = {};
    UINT32 m_ShadowWidth = 0;
    UINT32 m_ShadowHeight = 0;
    
    // Local Light Shadow Atlas (Spot/Point lights)
    ID3D11Texture2D* m_pLocalShadowAtlasTexture = nullptr;
    ID3D11DepthStencilView* m_pLocalShadowAtlasDSV = nullptr;
    ID3D11ShaderResourceView* m_pLocalShadowAtlasSRV = nullptr;
    D3D11_VIEWPORT m_LocalShadowViewport = {};
    UINT32 m_LocalShadowSize = 0;

private:
    bool createDevice(HWND hwnd);
    bool createSwapChain(HWND hwnd);
    bool createRenderTargets();
    bool createDepthStencil();

public:
    RSD3D11Device() = default;
    ~RSD3D11Device();

    bool initialize(HWND hwnd);
    void shutdown();
    void onResize(UINT32 width, UINT32 height);
    
    void clearRenderTarget(const float color[4]);
    void clearDepthStencil(float depth = 1.0f, UINT8 stencil = 0);
    void present(bool vsync = false);
    
    void setRenderTargets();
    
    // Directional Shadow Atlas
    bool createShadowAtlas(UINT32 size);
    void setShadowRenderTarget();
    void setCascadeViewport(UINT32 cascadeIndex);
    void clearShadowAtlas();
    
    // Local Light Shadow Atlas (8x8 grid for spot/point)
    bool createLocalShadowAtlas(UINT32 size);
    void setLocalShadowRenderTarget();
    void setLocalShadowSlotViewport(UINT32 slotIndex);  // slotIndex 0-63
    void clearLocalShadowAtlas();

    // Getters
    ID3D11Device* getDevice() const { return m_pDevice; }
    ID3D11DeviceContext* getContext() const { return m_pContext; }
    IDXGISwapChain* getSwapChain() const { return m_pSwapChain; }
    ID3D11RenderTargetView* getRenderTargetView() const { return m_pRenderTargetView; }
    ID3D11DepthStencilView* getDepthStencilView() const { return m_pDepthStencilView; }
    const D3D11_VIEWPORT& getViewport() const { return m_Viewport; }
    UINT32 getWidth() const { return m_Width; }
    UINT32 getHeight() const { return m_Height; }

    ID3D11ShaderResourceView* getShadowAtlasSRV() const { return m_pShadowAtlasSRV; }
    ID3D11ShaderResourceView* getLocalShadowAtlasSRV() const { return m_pLocalShadowAtlasSRV; }
};