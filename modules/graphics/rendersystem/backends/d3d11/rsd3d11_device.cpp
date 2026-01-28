#include "rsd3d11_device.h"
#include <iostream>

// ==================== DESTRUCTOR ====================
RSD3D11Device::~RSD3D11Device()
{
    shutdown();
}

// ==================== INITIALIZATION ====================
bool RSD3D11Device::initialize(HWND hwnd)
{
    if (!hwnd)
    {
        std::cerr << "[RSD3D11Device] ERROR: Invalid window handle.\n";
        return false;
    }

    std::cout << "[RSD3D11Device] Initializing...\n";

    if (!createDevice(hwnd))
    {
        std::cerr << "[RSD3D11Device] Failed to create device.\n";
        return false;
    }

    if (!createSwapChain(hwnd))
    {
        std::cerr << "[RSD3D11Device] Failed to create swap chain.\n";
        return false;
    }

    if (!createRenderTargets())
    {
        std::cerr << "[RSD3D11Device] Failed to create render targets.\n";
        return false;
    }

    if (!createDepthStencil())
    {
        std::cerr << "[RSD3D11Device] Failed to create depth stencil.\n";
        return false;
    }

    std::cout << "[RSD3D11Device] Initialization complete.\n";
    return true;
}

void RSD3D11Device::shutdown()
{
    std::cout << "[RSD3D11Device] Shutting down...\n";

    if (m_pDepthStencilBuffer) { m_pDepthStencilBuffer->Release(); m_pDepthStencilBuffer = nullptr; }
    if (m_pDepthStencilView) { m_pDepthStencilView->Release(); m_pDepthStencilView = nullptr; }
    if (m_pRenderTargetView) { m_pRenderTargetView->Release(); m_pRenderTargetView = nullptr; }
    if (m_pSwapChain) { m_pSwapChain->Release(); m_pSwapChain = nullptr; }
    if (m_pContext) { m_pContext->Release(); m_pContext = nullptr; }
    if (m_pDevice) { m_pDevice->Release(); m_pDevice = nullptr; }

    if (m_pShadowAtlasSRV) { m_pShadowAtlasSRV->Release(); m_pShadowAtlasSRV = nullptr; }
    if (m_pShadowAtlasDSV) { m_pShadowAtlasDSV->Release(); m_pShadowAtlasDSV = nullptr; }
    if (m_pShadowAtlasTexture) { m_pShadowAtlasTexture->Release(); m_pShadowAtlasTexture = nullptr; }

    // Local Shadow Atlas cleanup
    if (m_pLocalShadowAtlasSRV) { m_pLocalShadowAtlasSRV->Release(); m_pLocalShadowAtlasSRV = nullptr; }
    if (m_pLocalShadowAtlasDSV) { m_pLocalShadowAtlasDSV->Release(); m_pLocalShadowAtlasDSV = nullptr; }
    if (m_pLocalShadowAtlasTexture) { m_pLocalShadowAtlasTexture->Release(); m_pLocalShadowAtlasTexture = nullptr; }

    std::cout << "[RSD3D11Device] Shutdown complete.\n";
}

// ==================== DEVICE CREATION ====================
bool RSD3D11Device::createDevice(HWND hwnd)
{
    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };

    D3D_FEATURE_LEVEL featureLevel;

    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &m_pDevice,
        &featureLevel,
        &m_pContext
    );

    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11Device] D3D11CreateDevice failed with HRESULT: 0x"
            << std::hex << hr << std::dec << "\n";
        return false;
    }

    std::cout << "[RSD3D11Device] Device created with feature level: 0x"
        << std::hex << featureLevel << std::dec << "\n";
    return true;
}

bool RSD3D11Device::createSwapChain(HWND hwnd)
{
    RECT rect;
    GetClientRect(hwnd, &rect);
    m_Width = rect.right - rect.left;
    m_Height = rect.bottom - rect.top;

    if (m_Width == 0 || m_Height == 0)
    {
        std::cerr << "[RSD3D11Device] ERROR: Invalid window dimensions.\n";
        return false;
    }

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = m_Width;
    sd.BufferDesc.Height = m_Height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    IDXGIDevice* dxgiDevice = nullptr;
    HRESULT hr = m_pDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11Device] Failed to query DXGI device.\n";
        return false;
    }

    IDXGIAdapter* dxgiAdapter = nullptr;
    hr = dxgiDevice->GetAdapter(&dxgiAdapter);
    dxgiDevice->Release();

    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11Device] Failed to get DXGI adapter.\n";
        return false;
    }

    IDXGIFactory* dxgiFactory = nullptr;
    hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory);
    dxgiAdapter->Release();

    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11Device] Failed to get DXGI factory.\n";
        return false;
    }

    hr = dxgiFactory->CreateSwapChain(m_pDevice, &sd, &m_pSwapChain);
    dxgiFactory->Release();

    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11Device] CreateSwapChain failed.\n";
        return false;
    }

    std::cout << "[RSD3D11Device] Swap chain created (" << m_Width << "x" << m_Height << ")\n";
    return true;
}

bool RSD3D11Device::createRenderTargets()
{
    ID3D11Texture2D* backBuffer = nullptr;
    HRESULT hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11Device] Failed to get back buffer.\n";
        return false;
    }

    hr = m_pDevice->CreateRenderTargetView(backBuffer, nullptr, &m_pRenderTargetView);
    backBuffer->Release();

    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11Device] Failed to create render target view.\n";
        return false;
    }

    std::cout << "[RSD3D11Device] Render target view created.\n";
    return true;
}

bool RSD3D11Device::createDepthStencil()
{
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = m_Width;
    depthDesc.Height = m_Height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthDesc.CPUAccessFlags = 0;
    depthDesc.MiscFlags = 0;

    HRESULT hr = m_pDevice->CreateTexture2D(&depthDesc, nullptr, &m_pDepthStencilBuffer);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11Device] Failed to create depth stencil buffer.\n";
        return false;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = depthDesc.Format;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    hr = m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer, &dsvDesc, &m_pDepthStencilView);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11Device] Failed to create depth stencil view.\n";
        return false;
    }

    // Setup viewport
    m_Viewport.TopLeftX = 0.0f;
    m_Viewport.TopLeftY = 0.0f;
    m_Viewport.Width = static_cast<float>(m_Width);
    m_Viewport.Height = static_cast<float>(m_Height);
    m_Viewport.MinDepth = 0.0f;
    m_Viewport.MaxDepth = 1.0f;

    std::cout << "[RSD3D11Device] Depth stencil created.\n";
    return true;
}

// ==================== RESIZE ====================
void RSD3D11Device::onResize(UINT32 width, UINT32 height)
{
    if (!m_pSwapChain || width == 0 || height == 0)
    {
        std::cerr << "[RSD3D11Device] Invalid resize parameters.\n";
        return;
    }

    m_Width = width;
    m_Height = height;

    // Release old views
    if (m_pRenderTargetView) { m_pRenderTargetView->Release(); m_pRenderTargetView = nullptr; }
    if (m_pDepthStencilView) { m_pDepthStencilView->Release(); m_pDepthStencilView = nullptr; }
    if (m_pDepthStencilBuffer) { m_pDepthStencilBuffer->Release(); m_pDepthStencilBuffer = nullptr; }

    // Resize swap chain buffers
    HRESULT hr = m_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11Device] Failed to resize swap chain buffers.\n";
        return;
    }

    // Recreate views
    if (!createRenderTargets())
    {
        std::cerr << "[RSD3D11Device] Failed to recreate render targets after resize.\n";
        return;
    }

    if (!createDepthStencil())
    {
        std::cerr << "[RSD3D11Device] Failed to recreate depth stencil after resize.\n";
        return;
    }

    std::cout << "[RSD3D11Device] Resized to " << width << "x" << height << "\n";
}

// ==================== CLEAR & PRESENT ====================
void RSD3D11Device::clearRenderTarget(const float color[4])
{
    if (m_pContext && m_pRenderTargetView)
    {
        m_pContext->ClearRenderTargetView(m_pRenderTargetView, color);
    }
}

void RSD3D11Device::clearDepthStencil(float depth, UINT8 stencil)
{
    if (m_pContext && m_pDepthStencilView)
    {
        m_pContext->ClearDepthStencilView(m_pDepthStencilView, 
            D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depth, stencil);
    }
}

void RSD3D11Device::present(bool vsync)
{
    if (m_pSwapChain)
    {
        HRESULT hr = m_pSwapChain->Present(vsync ? 1 : 0, 0);
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            std::cerr << "[RSD3D11Device] FATAL: Device Lost or Reset. Reason: " 
                      << std::hex << (m_pDevice ? m_pDevice->GetDeviceRemovedReason() : hr) << std::dec << "\n";
            // TODO: Implement full device recovery/recreation here
            // For now, we just log it to avoid undefined behavior
        }
    }
}

void RSD3D11Device::setRenderTargets()
{
    if (m_pContext && m_pRenderTargetView && m_pDepthStencilView)
    {
        m_pContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);
        m_pContext->RSSetViewports(1, &m_Viewport);
    }
}

// ==================== SHADOW ATLAS IMPL ====================
bool RSD3D11Device::createShadowAtlas(UINT32 size)
{
    if (!m_pDevice) return false;

    m_ShadowWidth = size;
    m_ShadowHeight = size;

    // 1. Create Texture2D (Typeless for DSV and SRV compatibility)
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = size;
    texDesc.Height = size;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS; // 24-bit Depth, 8-bit Stencil (Typeless)
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;

    HRESULT hr = m_pDevice->CreateTexture2D(&texDesc, nullptr, &m_pShadowAtlasTexture);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11Device] Failed to create Shadow Atlas Texture.\n";
        return false;
    }

    // 2. Create Depth Stencil View (DSV)
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    hr = m_pDevice->CreateDepthStencilView(m_pShadowAtlasTexture, &dsvDesc, &m_pShadowAtlasDSV);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11Device] Failed to create Shadow Atlas DSV.\n";
        return false;
    }

    // 3. Create Shader Resource View (SRV) for sampling as texture
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; // Read depth as Red channel
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    hr = m_pDevice->CreateShaderResourceView(m_pShadowAtlasTexture, &srvDesc, &m_pShadowAtlasSRV);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11Device] Failed to create Shadow Atlas SRV.\n";
        return false;
    }

    // 4. Create Shadow Viewport
    m_ShadowViewport.TopLeftX = 0.0f;
    m_ShadowViewport.TopLeftY = 0.0f;
    m_ShadowViewport.Width = static_cast<float>(size);
    m_ShadowViewport.Height = static_cast<float>(size);
    m_ShadowViewport.MinDepth = 0.0f;
    m_ShadowViewport.MaxDepth = 1.0f;

    std::cout << "[RSD3D11Device] Shadow Atlas created (" << size << "x" << size << ").\n";
    return true;
}

void RSD3D11Device::setShadowRenderTarget()
{
    if (m_pContext && m_pShadowAtlasDSV)
    {
        // Unbind render targets (Depth Only Pass)
        ID3D11RenderTargetView* nullRTV = nullptr;
        m_pContext->OMSetRenderTargets(0, &nullRTV, m_pShadowAtlasDSV);
        m_pContext->RSSetViewports(1, &m_ShadowViewport);
    }
}

void RSD3D11Device::setCascadeViewport(UINT32 cascadeIndex)
{
    if (!m_pContext) return;
    
    // 2x2 grid layout: each cascade is half the atlas size
    float cascadeSize = static_cast<float>(m_ShadowWidth) / 2.0f;
    
    D3D11_VIEWPORT cascadeViewport = {};
    cascadeViewport.Width = cascadeSize;
    cascadeViewport.Height = cascadeSize;
    cascadeViewport.MinDepth = 0.0f;
    cascadeViewport.MaxDepth = 1.0f;
    
    // Grid positions: 0=(0,0), 1=(1,0), 2=(0,1), 3=(1,1)
    switch (cascadeIndex)
    {
        case 0: cascadeViewport.TopLeftX = 0.0f;         cascadeViewport.TopLeftY = 0.0f;         break;
        case 1: cascadeViewport.TopLeftX = cascadeSize;  cascadeViewport.TopLeftY = 0.0f;         break;
        case 2: cascadeViewport.TopLeftX = 0.0f;         cascadeViewport.TopLeftY = cascadeSize;  break;
        case 3: cascadeViewport.TopLeftX = cascadeSize;  cascadeViewport.TopLeftY = cascadeSize;  break;
        default: return;
    }
    
    m_pContext->RSSetViewports(1, &cascadeViewport);
}

void RSD3D11Device::clearShadowAtlas()
{
    if (m_pContext && m_pShadowAtlasDSV)
    {
        m_pContext->ClearDepthStencilView(m_pShadowAtlasDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
    }
}

// ==================== LOCAL SHADOW ATLAS (SPOT/POINT) ====================
bool RSD3D11Device::createLocalShadowAtlas(UINT32 size)
{
    if (!m_pDevice) return false;

    m_LocalShadowSize = size;

    // 1. Create Texture2D (Typeless for DSV and SRV compatibility)
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = size;
    texDesc.Height = size;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;

    HRESULT hr = m_pDevice->CreateTexture2D(&texDesc, nullptr, &m_pLocalShadowAtlasTexture);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11Device] Failed to create Local Shadow Atlas Texture.\n";
        return false;
    }

    // 2. Create Depth Stencil View (DSV)
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    hr = m_pDevice->CreateDepthStencilView(m_pLocalShadowAtlasTexture, &dsvDesc, &m_pLocalShadowAtlasDSV);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11Device] Failed to create Local Shadow Atlas DSV.\n";
        return false;
    }

    // 3. Create Shader Resource View (SRV)
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    hr = m_pDevice->CreateShaderResourceView(m_pLocalShadowAtlasTexture, &srvDesc, &m_pLocalShadowAtlasSRV);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11Device] Failed to create Local Shadow Atlas SRV.\n";
        return false;
    }

    // 4. Create default viewport (full atlas)
    m_LocalShadowViewport.TopLeftX = 0.0f;
    m_LocalShadowViewport.TopLeftY = 0.0f;
    m_LocalShadowViewport.Width = static_cast<float>(size);
    m_LocalShadowViewport.Height = static_cast<float>(size);
    m_LocalShadowViewport.MinDepth = 0.0f;
    m_LocalShadowViewport.MaxDepth = 1.0f;

    std::cout << "[RSD3D11Device] Local Shadow Atlas created (" << size << "x" << size << ", 8x8 grid).\n";
    return true;
}

void RSD3D11Device::setLocalShadowRenderTarget()
{
    if (m_pContext && m_pLocalShadowAtlasDSV)
    {
        ID3D11RenderTargetView* nullRTV = nullptr;
        m_pContext->OMSetRenderTargets(0, &nullRTV, m_pLocalShadowAtlasDSV);
        m_pContext->RSSetViewports(1, &m_LocalShadowViewport);
    }
}

void RSD3D11Device::setLocalShadowSlotViewport(UINT32 slotIndex)
{
    if (!m_pContext || slotIndex >= 64) return;
    
    // 8x8 grid layout
    constexpr UINT32 GRID_SIZE = 8;
    float slotSize = static_cast<float>(m_LocalShadowSize) / static_cast<float>(GRID_SIZE);
    
    UINT32 col = slotIndex % GRID_SIZE;
    UINT32 row = slotIndex / GRID_SIZE;
    
    D3D11_VIEWPORT slotViewport = {};
    slotViewport.TopLeftX = static_cast<float>(col) * slotSize;
    slotViewport.TopLeftY = static_cast<float>(row) * slotSize;
    slotViewport.Width = slotSize;
    slotViewport.Height = slotSize;
    slotViewport.MinDepth = 0.0f;
    slotViewport.MaxDepth = 1.0f;
    
    m_pContext->RSSetViewports(1, &slotViewport);
}

void RSD3D11Device::clearLocalShadowAtlas()
{
    if (m_pContext && m_pLocalShadowAtlasDSV)
    {
        m_pContext->ClearDepthStencilView(m_pLocalShadowAtlasDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
    }
}
