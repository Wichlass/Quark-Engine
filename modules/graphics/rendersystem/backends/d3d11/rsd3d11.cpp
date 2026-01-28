#include "rsd3d11.h"
#include "rsd3d11_device.h"
#include "rsd3d11_memory.h"
#include "rsd3d11_shaders.h"
#include "rsd3d11_pipeline.h"
#include "rsd3d11_math_converter.h"
#include <iostream>
#include <cmath>
#include <algorithm>

// stb_image for texture loading
#define STB_IMAGE_IMPLEMENTATION
#include "../../../../../thirdparty/assimp/contrib/stb/stb_image.h"

// Link D3D11 libraries
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

// ==================== CONSTRUCTOR / DESTRUCTOR ====================
RSD3D11::RSD3D11()
    : m_pDevice(nullptr)
    , m_pShaderManager(nullptr)
    , m_pPipelineManager(nullptr)
    , m_pMemoryManager(nullptr)
    , m_NextMeshHandle(1)
    , m_NextMaterialHandle(1)
    , m_NextTextureHandle(1)
    , m_pFrameConstantBuffer(nullptr)
    , m_pMaterialConstantBuffer(nullptr)
    , m_pInstanceBuffer(nullptr)
    , m_InstanceBufferSize(0)
    , m_pLightBuffer(nullptr)
    , m_pLightBufferSRV(nullptr)
    , m_pDefaultSampler(nullptr)
{
    std::cout << "[RSD3D11] Created.\n";
}

RSD3D11::~RSD3D11()
{
    shutdown();
}

// ==================== INITIALIZATION ====================
void RSD3D11::init(qWndh windowHandle)
{
    std::cout << "[RSD3D11] Initializing...\n";

    // Create device
    m_pDevice = std::make_unique<RSD3D11Device>();
    if (!m_pDevice->initialize(static_cast<HWND>(windowHandle)))
    {
        std::cerr << "[RSD3D11] ERROR: Failed to initialize device.\n";
        return;
    }

    ID3D11Device* device = m_pDevice->getDevice();
    ID3D11DeviceContext* context = m_pDevice->getContext();

    // Create memory manager
    m_pMemoryManager = std::make_unique<RSD3D11MemoryManager>();
    m_pMemoryManager->initialize(device, context);

    // Create shader manager
    m_pShaderManager = std::make_unique<RSD3D11ShaderManager>();
    m_pShaderManager->initialize(device, context);

    // Create pipeline manager
    m_pPipelineManager = std::make_unique<RSD3D11PipelineManager>();
    if (!m_pPipelineManager->initialize(device, context))
    {
        std::cerr << "[RSD3D11] ERROR: Failed to create pipeline manager.\n";
        return;
    }

    // Create frame constant buffer
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(FrameConstants) + 16 - (sizeof(FrameConstants) % 16);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&cbDesc, nullptr, &m_pFrameConstantBuffer);

    // Create material constant buffer
    cbDesc.ByteWidth = sizeof(MaterialData) + 16 - (sizeof(MaterialData) % 16);
    device->CreateBuffer(&cbDesc, nullptr, &m_pMaterialConstantBuffer);

    // Create default instance buffer (will resize as needed)
    createInstanceBuffer(1024 * sizeof(PerInstanceData));

    // Create default sampler
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    device->CreateSamplerState(&samplerDesc, &m_pDefaultSampler);

    // Create light structured buffer for dynamic lighting
    createLightBuffer();

    if (!m_pDevice->createShadowAtlas(static_cast<UINT32>(DIRECTIONAL_SHADOW_ATLAS_SIZE)))
    {
        std::cerr << "[RSD3D11] ERROR: Failed to create Shadow Atlas.\\n";
        return;
    }

    // Create local shadow atlas for spot/point lights (8x8 grid)
    if (!m_pDevice->createLocalShadowAtlas(static_cast<UINT32>(LOCAL_LIGHT_SHADOW_ATLAS_SIZE)))
    {
        std::cerr << "[RSD3D11] ERROR: Failed to create Local Shadow Atlas.\\n";
        return;
    }

    // Create sky sphere mesh
    if (!createSkySphere())
    {
        std::cerr << "[RSD3D11] WARNING: Failed to create sky sphere mesh.\n";
    }

    std::cout << "[RSD3D11] Initialized successfully.\n";
}

void RSD3D11::shutdown()
{
    std::cout << "[RSD3D11] Shutting down...\n";

    // Release mesh buffers
    for (auto& pair : m_MeshBuffers)
    {
        if (pair.second.pVertexBuffer) pair.second.pVertexBuffer->Release();
        if (pair.second.pIndexBuffer) pair.second.pIndexBuffer->Release();
    }
    m_MeshBuffers.clear();

    // Release material buffers
    for (auto& pair : m_MaterialBuffers)
    {
        if (pair.second.pConstantBuffer) pair.second.pConstantBuffer->Release();
        for (int i = 0; i < 6; ++i)
        {
            if (pair.second.textures[i]) pair.second.textures[i]->Release();
        }
    }
    m_MaterialBuffers.clear();

    // Release textures
    for (auto& pair : m_Textures)
    {
        if (pair.second.pTexture) pair.second.pTexture->Release();
        if (pair.second.pSRV) pair.second.pSRV->Release();
    }
    m_Textures.clear();

    if (m_pFrameConstantBuffer) { m_pFrameConstantBuffer->Release(); m_pFrameConstantBuffer = nullptr; }
    if (m_pMaterialConstantBuffer) { m_pMaterialConstantBuffer->Release(); m_pMaterialConstantBuffer = nullptr; }
    if (m_pInstanceBuffer) { m_pInstanceBuffer->Release(); m_pInstanceBuffer = nullptr; }
    if (m_pLightBuffer) { m_pLightBuffer->Release(); m_pLightBuffer = nullptr; }
    if (m_pLightBufferSRV) { m_pLightBufferSRV->Release(); m_pLightBufferSRV = nullptr; }
    if (m_pDefaultSampler) { m_pDefaultSampler->Release(); m_pDefaultSampler = nullptr; }
    if (m_pSkyVertexBuffer) { m_pSkyVertexBuffer->Release(); m_pSkyVertexBuffer = nullptr; }
    if (m_pSkyIndexBuffer) { m_pSkyIndexBuffer->Release(); m_pSkyIndexBuffer = nullptr; }

    // Shutdown managers
    if (m_pPipelineManager) m_pPipelineManager->shutdown();
    if (m_pShaderManager) m_pShaderManager->shutdown();
    if (m_pMemoryManager) m_pMemoryManager->shutdown();
    if (m_pDevice) m_pDevice->shutdown();

    m_pPipelineManager.reset();
    m_pShaderManager.reset();
    m_pMemoryManager.reset();
    m_pDevice.reset();

    std::cout << "[RSD3D11] Shutdown complete.\n";
}

// ==================== MESH BUFFER MANAGEMENT ====================
hMesh RSD3D11::createMeshBuffer(const MeshData& meshData, bool isDynamic)
{
    if (!m_pDevice) return 0;

    ID3D11Device* device = m_pDevice->getDevice();

    D3D11MeshBuffer buffer = {};
    buffer.vertexCount = meshData.vertexCount;
    buffer.indexCount = meshData.indexCount;
    buffer.vertexStride = sizeof(Vertex);
    buffer.isDynamic = isDynamic;

    // Create vertex buffer
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = meshData.vertexCount * sizeof(Vertex);
    vbDesc.Usage = isDynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = isDynamic ? D3D11_CPU_ACCESS_WRITE : 0;

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = meshData.vertices;

    HRESULT hr = device->CreateBuffer(&vbDesc, &vbData, &buffer.pVertexBuffer);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11] ERROR: Failed to create vertex buffer.\n";
        return 0;
    }

    // Create index buffer
    if (meshData.indices && meshData.indexCount > 0)
    {
        D3D11_BUFFER_DESC ibDesc = {};
        ibDesc.ByteWidth = meshData.indexCount * sizeof(UINT32);
        ibDesc.Usage = isDynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ibDesc.CPUAccessFlags = isDynamic ? D3D11_CPU_ACCESS_WRITE : 0;

        D3D11_SUBRESOURCE_DATA ibData = {};
        ibData.pSysMem = meshData.indices;

        hr = device->CreateBuffer(&ibDesc, &ibData, &buffer.pIndexBuffer);
        if (FAILED(hr))
        {
            buffer.pVertexBuffer->Release();
            std::cerr << "[RSD3D11] ERROR: Failed to create index buffer.\n";
            return 0;
        }
    }

    hMesh handle = m_NextMeshHandle++;
    m_MeshBuffers[handle] = buffer;
    return handle;
}

void RSD3D11::destroyMeshBuffer(hMesh handle)
{
    auto it = m_MeshBuffers.find(handle);
    if (it == m_MeshBuffers.end()) return;

    if (it->second.pVertexBuffer) it->second.pVertexBuffer->Release();
    if (it->second.pIndexBuffer) it->second.pIndexBuffer->Release();
    m_MeshBuffers.erase(it);
}

bool RSD3D11::updateMeshBuffer(hMesh handle, const MeshData& meshData)
{
    auto it = m_MeshBuffers.find(handle);
    if (it == m_MeshBuffers.end() || !it->second.isDynamic) return false;

    ID3D11DeviceContext* context = m_pDevice->getContext();

    // Update vertex buffer
    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = context->Map(it->second.pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr))
    {
        memcpy(mapped.pData, meshData.vertices, meshData.vertexCount * sizeof(Vertex));
        context->Unmap(it->second.pVertexBuffer, 0);
    }

    // Update index buffer if present
    if (it->second.pIndexBuffer && meshData.indices)
    {
        hr = context->Map(it->second.pIndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (SUCCEEDED(hr))
        {
            memcpy(mapped.pData, meshData.indices, meshData.indexCount * sizeof(UINT32));
            context->Unmap(it->second.pIndexBuffer, 0);
        }
    }

    it->second.vertexCount = meshData.vertexCount;
    it->second.indexCount = meshData.indexCount;
    return true;
}

// ==================== MATERIAL BUFFER MANAGEMENT ====================
hMaterial RSD3D11::createMaterialBuffer(const MaterialData& materialData)
{
    if (!m_pDevice) return 0;

    ID3D11Device* device = m_pDevice->getDevice();

    D3D11MaterialBuffer buffer = {};
    buffer.data = materialData;
    memset(buffer.textures, 0, sizeof(buffer.textures));

    // Create constant buffer
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(MaterialData) + 16 - (sizeof(MaterialData) % 16);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA cbData = {};
    cbData.pSysMem = &materialData;

    HRESULT hr = device->CreateBuffer(&cbDesc, &cbData, &buffer.pConstantBuffer);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11] ERROR: Failed to create material constant buffer.\n";
        return 0;
    }

    hMaterial handle = m_NextMaterialHandle++;
    m_MaterialBuffers[handle] = buffer;
    return handle;
}

void RSD3D11::destroyMaterialBuffer(hMaterial handle)
{
    auto it = m_MaterialBuffers.find(handle);
    if (it == m_MaterialBuffers.end()) return;

    if (it->second.pConstantBuffer) it->second.pConstantBuffer->Release();
    // Note: textures are shared, don't release here
    m_MaterialBuffers.erase(it);
}

bool RSD3D11::updateMaterialBuffer(hMaterial handle, const MaterialData& materialData)
{
    auto it = m_MaterialBuffers.find(handle);
    if (it == m_MaterialBuffers.end()) return false;

    ID3D11DeviceContext* context = m_pDevice->getContext();

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = context->Map(it->second.pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr))
    {
        memcpy(mapped.pData, &materialData, sizeof(MaterialData));
        context->Unmap(it->second.pConstantBuffer, 0);
        it->second.data = materialData;
        return true;
    }
    return false;
}

// ==================== TEXTURE MANAGEMENT ====================
hTexture RSD3D11::loadTexture(const char* filename)
{
    if (!m_pDevice || !filename)
    {
        std::cerr << "[RSD3D11] ERROR: Invalid device or filename for texture loading.\n";
        return 0;
    }

    // Load image with stb_image
    int width, height, channels;
    stbi_set_flip_vertically_on_load(false);  // D3D11 uses top-left origin
    unsigned char* imageData = stbi_load(filename, &width, &height, &channels, 4);  // Force RGBA
    
    if (!imageData)
    {
        std::cerr << "[RSD3D11] ERROR: Failed to load texture: " << filename << " - " << stbi_failure_reason() << "\n";
        return 0;
    }

    ID3D11Device* device = m_pDevice->getDevice();
    ID3D11DeviceContext* context = m_pDevice->getContext();
    
    // Create texture description
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = static_cast<UINT>(width);
    texDesc.Height = static_cast<UINT>(height);
    texDesc.MipLevels = 0; // Generate full mip chain
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    // Create texture (no initial data for autogen mips)
    D3D11Texture tex = {};
    HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, &tex.pTexture);
    
    if (FAILED(hr))
    {
        stbi_image_free(imageData);
        std::cerr << "[RSD3D11] ERROR: Failed to create D3D11 texture for: " << filename << "\n";
        return 0;
    }

    // Create shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = -1; // Use all mips

    hr = device->CreateShaderResourceView(tex.pTexture, &srvDesc, &tex.pSRV);
    if (FAILED(hr))
    {
        tex.pTexture->Release();
        stbi_image_free(imageData);
        std::cerr << "[RSD3D11] ERROR: Failed to create SRV for: " << filename << "\n";
        return 0;
    }

    // Upload level 0 data
    context->UpdateSubresource(tex.pTexture, 0, nullptr, imageData, width * 4, 0);

    // Generate mipmaps
    context->GenerateMips(tex.pSRV);

    // Free image data
    stbi_image_free(imageData);

    hTexture handle = m_NextTextureHandle++;
    m_Textures[handle] = tex;
    
    std::cout << "[RSD3D11] Loaded texture: " << filename << " (" << width << "x" << height << ") with Mipmaps.\n";
    return handle;
}

void RSD3D11::destroyTexture(hTexture handle)
{
    auto it = m_Textures.find(handle);
    if (it == m_Textures.end()) return;

    if (it->second.pTexture) it->second.pTexture->Release();
    if (it->second.pSRV) it->second.pSRV->Release();
    m_Textures.erase(it);
}

bool RSD3D11::bindTextureToMaterial(hMaterial material, hTexture texture, UINT32 slot)
{
    if (slot >= 6) return false;

    auto matIt = m_MaterialBuffers.find(material);
    if (matIt == m_MaterialBuffers.end()) return false;

    auto texIt = m_Textures.find(texture);
    if (texIt == m_Textures.end()) return false;

    matIt->second.textures[slot] = texIt->second.pSRV;
    return true;
}

// ==================== SHADOW PASS ====================
void RSD3D11::renderShadowPass(const FramePacket& packet)
{
    if (!m_pDevice || packet.shadowDrawCommandCount == 0 || packet.lightCount == 0) return;

    ID3D11DeviceContext* context = m_pDevice->getContext();

    // 1. Find shadow casting directional light
    const GPULightData* pShadowLight = nullptr;
    for (UINT32 i = 0; i < packet.lightCount; ++i)
    {
        if ((packet.lights[i].flags & static_cast<UINT32>(LightFlags::LIGHT_CAST_SHADOWS)) && 
            packet.lights[i].type == static_cast<UINT32>(LightType::DIRECTIONAL))
        {
            pShadowLight = &packet.lights[i];
            break;
        }
    }

    if (!pShadowLight) return;

    // 2. Setup Shadow Pipeline
    m_pDevice->setShadowRenderTarget();
    m_pDevice->clearShadowAtlas();
    
    m_pShaderManager->bindShadowPipeline();
    m_pPipelineManager->setShadowRasterizer();

    // 3. Upload shadow instance data (shared for all cascades)
    if (packet.shadowInstanceDataCount > 0)
    {
        size_t requiredSize = packet.shadowInstanceDataCount * sizeof(PerInstanceData);
        resizeInstanceBufferIfNeeded(requiredSize);
        
        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = context->Map(m_pInstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (SUCCEEDED(hr))
        {
            PerInstanceData* dest = static_cast<PerInstanceData*>(mapped.pData);
            for (UINT32 i = 0; i < packet.shadowInstanceDataCount; ++i)
            {
                dest[i] = convertInstanceData(packet.shadowInstanceData[i]);
            }
            context->Unmap(m_pInstanceBuffer, 0);
        }
    }

    // D3D11 Z correction is now handled directly in computeCascadeMatrix (csm.h)
    // No manual correction needed here.

    // 4. Render each cascade
    for (UINT32 cascade = 0; cascade < DIRECTIONAL_CASCADE_COUNT; ++cascade)
    {
        // Set cascade viewport
        m_pDevice->setCascadeViewport(cascade);

        // Setup constants for this cascade
        // Matrix comes as [-1, 1] from csm.h, convert to [0, 1] and transpose for D3D
        FrameConstants shadowConstants = convertFrameConstants(packet.constants);
        shadowConstants.viewProjection = convertShadowProjectionMatrix(pShadowLight->cascadeMatrices[cascade]);
        
        m_pPipelineManager->updateFrameConstants(shadowConstants);
        m_pPipelineManager->bindFrameConstants();

        // Draw all shadow geometry
        hMesh lastMesh = 0;
        UINT instanceStride = sizeof(PerInstanceData);

        for (UINT32 i = 0; i < packet.shadowDrawCommandCount; ++i)
        {
            const DrawCommand& cmd = packet.shadowDrawCommands[i];
            
            auto meshIt = m_MeshBuffers.find(cmd.mesh);
            if (meshIt == m_MeshBuffers.end()) continue;

            if (cmd.mesh != lastMesh)
            {
                ID3D11Buffer* buffers[2] = { meshIt->second.pVertexBuffer, m_pInstanceBuffer };
                UINT strides[2] = { meshIt->second.vertexStride, instanceStride };
                UINT offsets[2] = { 0, 0 };
                context->IASetVertexBuffers(0, 2, buffers, strides, offsets);
                context->IASetIndexBuffer(meshIt->second.pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
                lastMesh = cmd.mesh;
            }

            context->DrawIndexedInstanced(meshIt->second.indexCount, cmd.instanceCount, 0, 0, cmd.instanceStart);
        }
    }
}

// ==================== SPOT SHADOW PASS ====================
void RSD3D11::renderSpotShadowPass(const FramePacket& packet)
{
    if (!m_pDevice || packet.shadowDrawCommandCount == 0 || packet.lightCount == 0) return;

    ID3D11DeviceContext* context = m_pDevice->getContext();

    // Count spot lights that need shadow rendering
    UINT32 spotShadowCount = 0;
    for (UINT32 i = 0; i < packet.lightCount; ++i)
    {
        if ((packet.lights[i].flags & static_cast<UINT32>(LightFlags::LIGHT_CAST_SHADOWS)) &&
            packet.lights[i].type == static_cast<UINT32>(LightType::SPOT) &&
            packet.lights[i].shadowIndex >= 0)
        {
            spotShadowCount++;
        }
    }

    if (spotShadowCount == 0) return;

    // Local shadow atlas already cleared in executeFrame before all local shadow passes
    m_pDevice->setLocalShadowRenderTarget();
    
    // Use shadow pipeline (depth-only shader)
    m_pShaderManager->bindShadowPipeline();
    m_pPipelineManager->setShadowRasterizer();

    // Upload shadow instance data (shared for all lights)
    if (packet.shadowInstanceDataCount > 0)
    {
        size_t requiredSize = packet.shadowInstanceDataCount * sizeof(PerInstanceData);
        resizeInstanceBufferIfNeeded(requiredSize);
        
        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = context->Map(m_pInstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (SUCCEEDED(hr))
        {
            PerInstanceData* dest = static_cast<PerInstanceData*>(mapped.pData);
            for (UINT32 i = 0; i < packet.shadowInstanceDataCount; ++i)
            {
                dest[i] = convertInstanceData(packet.shadowInstanceData[i]);
            }
            context->Unmap(m_pInstanceBuffer, 0);
        }
    }

    // Render shadow map for each spot light
    for (UINT32 lightIdx = 0; lightIdx < packet.lightCount; ++lightIdx)
    {
        const GPULightData& light = packet.lights[lightIdx];
        
        // Skip non-shadow-casting and non-spot lights
        if (!(light.flags & static_cast<UINT32>(LightFlags::LIGHT_CAST_SHADOWS)) ||
            light.type != static_cast<UINT32>(LightType::SPOT) ||
            light.shadowIndex < 0)
        {
            continue;
        }

        // Set viewport for this spot light's atlas slot
        m_pDevice->setLocalShadowSlotViewport(static_cast<UINT32>(light.shadowIndex));

        // Setup constants with spot light's shadow matrix
        // Convert to D3D11 format (RH column-major -> LH row-major)
        FrameConstants shadowConstants = convertFrameConstants(packet.constants);
        shadowConstants.viewProjection = convertShadowProjectionMatrix(light.spotShadowMatrix);
        
        m_pPipelineManager->updateFrameConstants(shadowConstants);
        m_pPipelineManager->bindFrameConstants();

        // Draw all shadow geometry
        hMesh lastMesh = 0;
        UINT instanceStride = sizeof(PerInstanceData);

        for (UINT32 cmdIdx = 0; cmdIdx < packet.shadowDrawCommandCount; ++cmdIdx)
        {
            const DrawCommand& cmd = packet.shadowDrawCommands[cmdIdx];
            
            auto meshIt = m_MeshBuffers.find(cmd.mesh);
            if (meshIt == m_MeshBuffers.end()) continue;

            if (cmd.mesh != lastMesh)
            {
                ID3D11Buffer* buffers[2] = { meshIt->second.pVertexBuffer, m_pInstanceBuffer };
                UINT strides[2] = { meshIt->second.vertexStride, instanceStride };
                UINT offsets[2] = { 0, 0 };
                context->IASetVertexBuffers(0, 2, buffers, strides, offsets);
                context->IASetIndexBuffer(meshIt->second.pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
                lastMesh = cmd.mesh;
            }

            context->DrawIndexedInstanced(meshIt->second.indexCount, cmd.instanceCount, 0, 0, cmd.instanceStart);
        }
    }
}

// ==================== POINT SHADOW PASS (CUBE MAP) ====================
void RSD3D11::renderPointShadowPass(const FramePacket& packet)
{
    if (!m_pDevice || packet.shadowDrawCommandCount == 0 || packet.lightCount == 0) return;

    ID3D11DeviceContext* context = m_pDevice->getContext();

    // Count point lights that need shadow rendering
    UINT32 pointShadowCount = 0;
    for (UINT32 i = 0; i < packet.lightCount; ++i)
    {
        if ((packet.lights[i].flags & static_cast<UINT32>(LightFlags::LIGHT_CAST_SHADOWS)) &&
            packet.lights[i].type == static_cast<UINT32>(LightType::POINT) &&
            packet.lights[i].shadowIndex >= 0)
        {
            pointShadowCount++;
        }
    }

    if (pointShadowCount == 0) return;

    // Local shadow atlas should already be set from spot light pass
    // If no spot lights rendered, we need to set it here
    m_pDevice->setLocalShadowRenderTarget();
    
    // Use shadow pipeline (depth-only shader)
    m_pShaderManager->bindShadowPipeline();
    m_pPipelineManager->setShadowRasterizer();

    // Upload shadow instance data (shared for all lights)
    if (packet.shadowInstanceDataCount > 0)
    {
        size_t requiredSize = packet.shadowInstanceDataCount * sizeof(PerInstanceData);
        resizeInstanceBufferIfNeeded(requiredSize);
        
        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = context->Map(m_pInstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (SUCCEEDED(hr))
        {
            PerInstanceData* dest = static_cast<PerInstanceData*>(mapped.pData);
            for (UINT32 i = 0; i < packet.shadowInstanceDataCount; ++i)
            {
                dest[i] = convertInstanceData(packet.shadowInstanceData[i]);
            }
            context->Unmap(m_pInstanceBuffer, 0);
        }
    }

    // Render shadow map for each point light (6 faces each)
    for (UINT32 lightIdx = 0; lightIdx < packet.lightCount; ++lightIdx)
    {
        const GPULightData& light = packet.lights[lightIdx];
        
        // Skip non-shadow-casting and non-point lights
        if (!(light.flags & static_cast<UINT32>(LightFlags::LIGHT_CAST_SHADOWS)) ||
            light.type != static_cast<UINT32>(LightType::POINT) ||
            light.shadowIndex < 0)
        {
            continue;
        }

        // Render all 6 cube faces
        for (UINT32 faceIdx = 0; faceIdx < POINT_SHADOW_FACE_COUNT; ++faceIdx)
        {
            // Calculate slot index for this face
            // shadowIndex is the base slot, add faceIdx for each face
            UINT32 slotIndex = static_cast<UINT32>(light.shadowIndex) + faceIdx;
            
            // Set viewport for this face's atlas slot
            m_pDevice->setLocalShadowSlotViewport(slotIndex);

            // Setup constants with this face's shadow matrix
            FrameConstants shadowConstants = convertFrameConstants(packet.constants);
            shadowConstants.viewProjection = convertShadowProjectionMatrix(light.pointShadowMatrices[faceIdx]);
            
            m_pPipelineManager->updateFrameConstants(shadowConstants);
            m_pPipelineManager->bindFrameConstants();

            // Draw all shadow geometry
            hMesh lastMesh = 0;
            UINT instanceStride = sizeof(PerInstanceData);

            for (UINT32 cmdIdx = 0; cmdIdx < packet.shadowDrawCommandCount; ++cmdIdx)
            {
                const DrawCommand& cmd = packet.shadowDrawCommands[cmdIdx];
                
                auto meshIt = m_MeshBuffers.find(cmd.mesh);
                if (meshIt == m_MeshBuffers.end()) continue;

                if (cmd.mesh != lastMesh)
                {
                    ID3D11Buffer* buffers[2] = { meshIt->second.pVertexBuffer, m_pInstanceBuffer };
                    UINT strides[2] = { meshIt->second.vertexStride, instanceStride };
                    UINT offsets[2] = { 0, 0 };
                    context->IASetVertexBuffers(0, 2, buffers, strides, offsets);
                    context->IASetIndexBuffer(meshIt->second.pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
                    lastMesh = cmd.mesh;
                }

                context->DrawIndexedInstanced(meshIt->second.indexCount, cmd.instanceCount, 0, 0, cmd.instanceStart);
            }
        }
    }
}

// ==================== RENDER SKY ====================
void RSD3D11::renderSky(const FramePacket& packet)
{
    if (!m_pDevice || !m_pSkyVertexBuffer || !m_pSkyIndexBuffer || m_SkyIndexCount == 0) return;
    
    ID3D11DeviceContext* context = m_pDevice->getContext();
    
    // 1. Set sky pipeline
    m_pShaderManager->bindSkyPipeline();
    
    // 2. Set sky depth stencil state (test but no write, LESS_EQUAL for far plane)
    m_pPipelineManager->setSkyDepthStencilState();
    
    // 3. Set rasterizer - cull front faces (we're inside the sphere)
    m_pPipelineManager->setRasterizerState(CullMode::FRONT);
    
    // 4. Update and bind sky constant buffer
    m_pPipelineManager->updateSkyConstants(packet.skySettings, packet.constants.time);
    m_pPipelineManager->bindSkyConstants();
    
    // 5. Bind frame constants (use RSD3D11's buffer which has transposed matrices!)
    // m_pFrameConstantBuffer contains the properly converted matrices from uploadFrameConstants
    context->VSSetConstantBuffers(0, 1, &m_pFrameConstantBuffer);
    context->PSSetConstantBuffers(0, 1, &m_pFrameConstantBuffer);
    
    // 6. Bind sky sphere mesh
    ID3D11Buffer* buffers[1] = { m_pSkyVertexBuffer };
    UINT strides[1] = { sizeof(Vertex) };
    UINT offsets[1] = { 0 };
    context->IASetVertexBuffers(0, 1, buffers, strides, offsets);
    context->IASetIndexBuffer(m_pSkyIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    context->IASetInputLayout(m_pShaderManager->getPBRInputLayout()); // Use PBR layout for vertex format
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    // 7. Draw sky sphere
    context->DrawIndexed(m_SkyIndexCount, 0, 0);
    
    // 8. Restore default depth stencil state for next pass
    m_pPipelineManager->setDefaultDepthStencilState();
    m_pPipelineManager->setRasterizerState(CullMode::BACK);
}

// ==================== FRAME EXECUTION ====================
void RSD3D11::executeFrame(const FramePacket& packet)
{
    if (!m_pDevice) return;

    ID3D11DeviceContext* context = m_pDevice->getContext();

    // Clear render target
    ID3D11RenderTargetView* rtv = m_pDevice->getRenderTargetView();
    ID3D11DepthStencilView* dsv = m_pDevice->getDepthStencilView();
    
    context->ClearRenderTargetView(rtv, packet.clearColor);
    context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    // Set render target
    context->OMSetRenderTargets(1, &rtv, dsv);

    // Set viewport
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(packet.viewportWidth > 0 ? packet.viewportWidth : m_pDevice->getWidth());
    viewport.Height = static_cast<float>(packet.viewportHeight > 0 ? packet.viewportHeight : m_pDevice->getHeight());
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);

    // Set primitive topology
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Upload frame constants
    uploadFrameConstants(packet);

    // Upload instance data
    uploadInstanceData(packet);

    // Upload light data
    uploadLightData(packet);

    // 1. Shadow Passes
    renderShadowPass(packet);       // Directional CSM
    
    // Clear local shadow atlas once before spot and point passes
    m_pDevice->setLocalShadowRenderTarget();
    m_pDevice->clearLocalShadowAtlas();
    
    renderSpotShadowPass(packet);   // Spot light shadows
    renderPointShadowPass(packet);  // Point light cube map shadows

    // 2. Main Pass
    // Restore Main Render Target & Viewport
    context->OMSetRenderTargets(1, &rtv, dsv);
    
    // Reuse existing viewport variable
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(packet.viewportWidth > 0 ? packet.viewportWidth : m_pDevice->getWidth());
    viewport.Height = static_cast<float>(packet.viewportHeight > 0 ? packet.viewportHeight : m_pDevice->getHeight());
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);

    // RESTORE Frame Constants (Camera Matrices)
    // renderShadowPass overwrote them with Light Matrices. We must restore them !
    uploadFrameConstants(packet);

    // RESTORE Main Instance Data
    // renderShadowPass uploaded shadow instance data, we must restore main pass data
    uploadInstanceData(packet);

    // Execute draw commands
    executeDrawCommands(packet);

    // 3. Sky Pass (render after main geometry, at far plane)
    renderSky(packet);

    // Note: present() called separately by RenderSystem
}

void RSD3D11::endFrame()
{
    if (m_pDevice)
    {
        m_pDevice->present();
    }
}

void RSD3D11::uploadFrameConstants(const FramePacket& packet)
{
    ID3D11DeviceContext* context = m_pDevice->getContext();

    // Convert matrices to D3D11 format (column-major -> row-major)
    FrameConstants d3d11Constants = convertFrameConstants(packet.constants);

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = context->Map(m_pFrameConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr))
    {
        memcpy(mapped.pData, &d3d11Constants, sizeof(FrameConstants));
        context->Unmap(m_pFrameConstantBuffer, 0);
    }

    // Bind to VS and PS
    context->VSSetConstantBuffers(0, 1, &m_pFrameConstantBuffer);
    context->PSSetConstantBuffers(0, 1, &m_pFrameConstantBuffer);
}

void RSD3D11::uploadInstanceData(const FramePacket& packet)
{
    if (packet.instanceDataCount == 0) return;

    size_t requiredSize = packet.instanceDataCount * sizeof(PerInstanceData);
    resizeInstanceBufferIfNeeded(requiredSize);

    ID3D11DeviceContext* context = m_pDevice->getContext();

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = context->Map(m_pInstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr))
    {
        // Convert each instance data to D3D11 format
        PerInstanceData* dest = static_cast<PerInstanceData*>(mapped.pData);
        for (UINT32 i = 0; i < packet.instanceDataCount; ++i)
        {
            dest[i] = convertInstanceData(packet.instanceData[i]);
        }
        context->Unmap(m_pInstanceBuffer, 0);
    }
}

void RSD3D11::executeDrawCommands(const FramePacket& packet)
{
    if (packet.drawCommandCount == 0) return;

    ID3D11DeviceContext* context = m_pDevice->getContext();

    // Set shaders and pipeline state
    m_pShaderManager->bindPBRPipeline();
    
    // Set default depth stencil and blend states
    m_pPipelineManager->setDefaultDepthStencilState();
    m_pPipelineManager->setOpaqueBlendState();

    // Set sampler
    m_pPipelineManager->bindSamplers(); // Bind Linear (s0) and Shadow (s1)

    // Bind Shadow Atlases to PS
    ID3D11ShaderResourceView* shadowSRV = m_pDevice->getShadowAtlasSRV();
    ID3D11ShaderResourceView* localShadowSRV = m_pDevice->getLocalShadowAtlasSRV();
    context->PSSetShaderResources(10, 1, &shadowSRV);      // t10: Directional CSM
    context->PSSetShaderResources(11, 1, &localShadowSRV); // t11: Local (Spot/Point)

    // Set instance buffer
    UINT instanceStride = sizeof(PerInstanceData);
    
    // State caching to avoid redundant GPU binds
    hMesh lastMesh = 0;
    hMaterial lastMaterial = 0;
    CullMode lastCullMode = static_cast<CullMode>(UINT32_MAX);  // Force first set
    
    for (UINT32 i = 0; i < packet.drawCommandCount; ++i)
    {
        const DrawCommand& cmd = packet.drawCommands[i];

        // Find mesh buffer
        auto meshIt = m_MeshBuffers.find(cmd.mesh);
        if (meshIt == m_MeshBuffers.end()) continue;

        // Find material buffer
        auto matIt = m_MaterialBuffers.find(cmd.material);
        if (matIt == m_MaterialBuffers.end()) continue;

        // Bind mesh only if changed
        if (cmd.mesh != lastMesh)
        {
            ID3D11Buffer* buffers[2] = { meshIt->second.pVertexBuffer, m_pInstanceBuffer };
            UINT strides[2] = { meshIt->second.vertexStride, instanceStride };
            UINT offsets[2] = { 0, 0 };
            context->IASetVertexBuffers(0, 2, buffers, strides, offsets);
            context->IASetIndexBuffer(meshIt->second.pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
            lastMesh = cmd.mesh;
        }

        // Bind material only if changed
        if (cmd.material != lastMaterial)
        {
            context->VSSetConstantBuffers(1, 1, &matIt->second.pConstantBuffer);
            context->PSSetConstantBuffers(1, 1, &matIt->second.pConstantBuffer);
            context->PSSetShaderResources(0, 6, matIt->second.textures);
            
            // Set cull mode from material
            CullMode cullMode = static_cast<CullMode>(matIt->second.data.cullMode);
            if (cullMode != lastCullMode)
            {
                m_pPipelineManager->setRasterizerState(cullMode);
                lastCullMode = cullMode;
            }
            
            lastMaterial = cmd.material;
        }

        // Draw instanced - use StartInstanceLocation for instance buffer offset
        context->DrawIndexedInstanced(meshIt->second.indexCount, cmd.instanceCount, 0, 0, cmd.instanceStart);
    }
}

bool RSD3D11::createInstanceBuffer(size_t size)
{
    if (!m_pDevice) return false;
    if (m_pInstanceBuffer) m_pInstanceBuffer->Release();

    ID3D11Device* device = m_pDevice->getDevice();

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = static_cast<UINT>(size);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = device->CreateBuffer(&desc, nullptr, &m_pInstanceBuffer);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11] ERROR: Failed to create instance buffer.\n";
        return false;
    }

    m_InstanceBufferSize = size;
    return true;
}

bool RSD3D11::resizeInstanceBufferIfNeeded(size_t requiredSize)
{
    if (requiredSize <= m_InstanceBufferSize) return true;

    // Double the size to avoid frequent reallocations
    size_t newSize = m_InstanceBufferSize * 2;
    if (newSize < requiredSize) newSize = requiredSize;

    return createInstanceBuffer(newSize);
}

void RSD3D11::onResize(UINT32 width, UINT32 height)
{
    if (m_pDevice)
    {
        m_pDevice->onResize(width, height);
    }
}

void* RSD3D11::getDevice() const
{
    return m_pDevice ? m_pDevice->getDevice() : nullptr;
}

void* RSD3D11::getContext() const
{
    return m_pDevice ? m_pDevice->getContext() : nullptr;
}

// ==================== LIGHT BUFFER ====================
bool RSD3D11::createLightBuffer()
{
    if (!m_pDevice) return false;

    ID3D11Device* device = m_pDevice->getDevice();

    // Create structured buffer for lights
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = MAX_LIGHTS * sizeof(GPULightData);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufferDesc.StructureByteStride = sizeof(GPULightData);

    HRESULT hr = device->CreateBuffer(&bufferDesc, nullptr, &m_pLightBuffer);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11] ERROR: Failed to create light buffer.\\n";
        return false;
    }

    // Create SRV for the light buffer
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = MAX_LIGHTS;

    hr = device->CreateShaderResourceView(m_pLightBuffer, &srvDesc, &m_pLightBufferSRV);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11] ERROR: Failed to create light buffer SRV.\\n";
        return false;
    }

    std::cout << "[RSD3D11] Light buffer created (max " << MAX_LIGHTS << " lights).\n";
    return true;
}

void RSD3D11::uploadLightData(const FramePacket& packet)
{
    if (!m_pDevice || !m_pLightBuffer) return;
    if (packet.lightCount == 0) return;

    ID3D11DeviceContext* context = m_pDevice->getContext();

    // Create Local Copy of Lights with D3D11 conversions
    if (packet.lightCount > 0)
    {
        std::vector<GPULightData> convertedLights;
        convertedLights.reserve(packet.lightCount);
        
        // D3D11 Z correction is now handled directly in computeCascadeMatrix (csm.h)

        UINT32 dirShadowCount = 0;
        UINT32 spotShadowCount = 0;
        UINT32 pointShadowCount = 0;
        for (UINT32 i = 0; i < packet.lightCount; ++i)
        {
            // 1. Convert to D3D11 format
            // Light Data contains general matrices, but Cascade Matrices are Projections.
            // We must convert them using shadow projection converter.
            GPULightData light = convertLightData(packet.lights[i]);
            for (UINT32 c = 0; c < DIRECTIONAL_CASCADE_COUNT; ++c)
            {
                light.cascadeMatrices[c] = convertShadowProjectionMatrix(packet.lights[i].cascadeMatrices[c]);
            }
            
            // Convert spot shadow matrix to D3D11 format
            if (light.type == static_cast<UINT32>(LightType::SPOT) && light.shadowIndex >= 0)
            {
                light.spotShadowMatrix = convertShadowProjectionMatrix(packet.lights[i].spotShadowMatrix);
            }
            
            // Convert point shadow matrices to D3D11 format (6 cube faces)
            if (light.type == static_cast<UINT32>(LightType::POINT) && light.shadowIndex >= 0)
            {
                for (UINT32 f = 0; f < POINT_SHADOW_FACE_COUNT; ++f)
                {
                    light.pointShadowMatrices[f] = convertShadowProjectionMatrix(packet.lights[i].pointShadowMatrices[f]);
                }
            }
            
            if (light.flags & static_cast<UINT32>(LightFlags::LIGHT_CAST_SHADOWS))
            {
                bool keepShadow = false;

                if (light.type == static_cast<UINT32>(LightType::DIRECTIONAL))
                {
                    if (dirShadowCount < 1) 
                    {
                        dirShadowCount++;
                        keepShadow = true;
                    }
                }
                else if (light.type == static_cast<UINT32>(LightType::SPOT))
                {
                    if (spotShadowCount < MAX_SPOT_SHADOW_LIGHTS)
                    {
                        spotShadowCount++;
                        keepShadow = true;
                    }
                }
                else if (light.type == static_cast<UINT32>(LightType::POINT))
                {
                    if (pointShadowCount < MAX_POINT_SHADOW_LIGHTS)
                    {
                        pointShadowCount++;
                        keepShadow = true;
                    }
                }

                if (!keepShadow)
                {
                    light.flags &= ~static_cast<UINT32>(LightFlags::LIGHT_CAST_SHADOWS);
                }
            }

            convertedLights.push_back(light);
        }

        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = context->Map(m_pLightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (SUCCEEDED(hr))
        {
            memcpy(mapped.pData, convertedLights.data(), convertedLights.size() * sizeof(GPULightData));
            context->Unmap(m_pLightBuffer, 0);
        }
    }

    // Bind light buffer SRV to pixel shader slot t6
    context->PSSetShaderResources(6, 1, &m_pLightBufferSRV);
}

// ==================== SKY SPHERE ====================
bool RSD3D11::createSkySphere()
{
    if (!m_pDevice) return false;
    
    ID3D11Device* device = m_pDevice->getDevice();
    
    // Generate UV sphere with reasonable resolution
    const UINT32 slices = 32;  // longitude divisions
    const UINT32 stacks = 16;  // latitude divisions
    const float radius = 10.0f; // Sphere radius (arbitrary, shader handles positioning)
    
    // Calculate vertex and index counts
    UINT32 vertexCount = (slices + 1) * (stacks + 1);
    UINT32 indexCount = slices * stacks * 6;
    
    std::vector<Vertex> vertices(vertexCount);
    std::vector<UINT32> indices(indexCount);
    
    // Generate vertices
    UINT32 v = 0;
    for (UINT32 stack = 0; stack <= stacks; ++stack)
    {
        float phi = static_cast<float>(stack) / stacks * 3.14159265359f; // 0 to PI
        float sinPhi = sinf(phi);
        float cosPhi = cosf(phi);
        
        for (UINT32 slice = 0; slice <= slices; ++slice)
        {
            float theta = static_cast<float>(slice) / slices * 2.0f * 3.14159265359f; // 0 to 2*PI
            float sinTheta = sinf(theta);
            float cosTheta = cosf(theta);
            
            // Position on unit sphere
            float x = sinPhi * cosTheta;
            float y = cosPhi;
            float z = sinPhi * sinTheta;
            
            vertices[v].position = Quark::Vec3(x * radius, y * radius, z * radius);
            vertices[v].normal = Quark::Vec3(x, y, z); // Normal points outward
            vertices[v].texCoord = Quark::Vec2(
                static_cast<float>(slice) / slices,
                static_cast<float>(stack) / stacks
            );
            vertices[v].tangent = Quark::Vec3(-sinTheta, 0.0f, cosTheta);
            vertices[v].bitangent = Quark::Vec3(cosPhi * cosTheta, -sinPhi, cosPhi * sinTheta);
            
            ++v;
        }
    }
    
    // Generate indices
    UINT32 i = 0;
    for (UINT32 stack = 0; stack < stacks; ++stack)
    {
        for (UINT32 slice = 0; slice < slices; ++slice)
        {
            UINT32 first = stack * (slices + 1) + slice;
            UINT32 second = first + slices + 1;
            
            // Two triangles per quad
            // CCW winding: we're inside the sphere, so reverse winding
            indices[i++] = first;
            indices[i++] = first + 1;
            indices[i++] = second;
            
            indices[i++] = second;
            indices[i++] = first + 1;
            indices[i++] = second + 1;
        }
    }
    
    m_SkyIndexCount = indexCount;
    
    // Create vertex buffer
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = vertexCount * sizeof(Vertex);
    vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = 0;
    
    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices.data();
    
    HRESULT hr = device->CreateBuffer(&vbDesc, &vbData, &m_pSkyVertexBuffer);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11] Error: Failed to create sky vertex buffer.\n";
        return false;
    }
    
    // Create index buffer
    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.ByteWidth = indexCount * sizeof(UINT32);
    ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibDesc.CPUAccessFlags = 0;
    
    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices.data();
    
    hr = device->CreateBuffer(&ibDesc, &ibData, &m_pSkyIndexBuffer);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11] Error: Failed to create sky index buffer.\n";
        m_pSkyVertexBuffer->Release();
        m_pSkyVertexBuffer = nullptr;
        return false;
    }
    
    std::cout << "[RSD3D11] Sky sphere created: " << vertexCount << " vertices, " << indexCount << " indices.\n";
    return true;
}

// ==================== C API ====================
RHI* createRenderBackend()
{
    return new RSD3D11();
}

void destroyRenderBackend(RHI* rhi)
{
    if (rhi)
    {
        delete rhi;
    }
}
