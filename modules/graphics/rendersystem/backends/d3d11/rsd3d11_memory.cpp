#include "rsd3d11_memory.h"
#include <iostream>
#include <algorithm>

// ==================== CONSTRUCTOR/DESTRUCTOR ====================
RSD3D11MemoryManager::RSD3D11MemoryManager()
    : m_pDevice(nullptr)
    , m_pContext(nullptr)
    , m_Stats{}
{
}

RSD3D11MemoryManager::~RSD3D11MemoryManager()
{
    shutdown();
}

// ==================== LIFECYCLE ====================
bool RSD3D11MemoryManager::initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
    if (!device || !context)
    {
        std::cerr << "[RSD3D11MemoryManager] Invalid device or context.\n";
        return false;
    }
    
    m_pDevice = device;
    m_pContext = context;
    
    // Pre-allocate initial pools
    createPool(BufferType::BUFFER_VERTEX, DEFAULT_VERTEX_POOL_SIZE);
    createPool(BufferType::BUFFER_INDEX, DEFAULT_INDEX_POOL_SIZE);
    createPool(BufferType::BUFFER_CONSTANT, DEFAULT_CONSTANT_POOL_SIZE);
    createPool(BufferType::BUFFER_INSTANCE, DEFAULT_INSTANCE_POOL_SIZE);
    
    updateStats();
    std::cout << "[RSD3D11MemoryManager] Initialized with " << m_Stats.poolCount << " pools.\n";
    return true;
}

void RSD3D11MemoryManager::shutdown()
{
    std::cout << "[RSD3D11MemoryManager] Shutting down...\n";
    
    for (auto& pool : m_VertexPools)
        releasePool(pool);
    for (auto& pool : m_IndexPools)
        releasePool(pool);
    for (auto& pool : m_ConstantPools)
        releasePool(pool);
    for (auto& pool : m_InstancePools)
        releasePool(pool);
    
    m_VertexPools.clear();
    m_IndexPools.clear();
    m_ConstantPools.clear();
    m_InstancePools.clear();
    
    m_pDevice = nullptr;
    m_pContext = nullptr;
    
    std::cout << "[RSD3D11MemoryManager] Shutdown complete.\n";
}

// ==================== POOL MANAGEMENT ====================
BufferPool* RSD3D11MemoryManager::createPool(BufferType type, size_t size)
{
    if (!m_pDevice)
        return nullptr;
    
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = static_cast<UINT>(size);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.CPUAccessFlags = 0;
    
    switch (type)
    {
    case BufferType::BUFFER_VERTEX:
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        break;
    case BufferType::BUFFER_INDEX:
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        break;
    case BufferType::BUFFER_CONSTANT:
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        break;
    case BufferType::BUFFER_INSTANCE:
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        break;
    }
    
    BufferPool pool = {};
    pool.type = type;
    pool.totalSize = size;
    pool.usedSize = 0;
    
    HRESULT hr = m_pDevice->CreateBuffer(&desc, nullptr, &pool.pBuffer);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11MemoryManager] Failed to create buffer pool.\n";
        return nullptr;
    }
    
    // Başlangıçta tüm alan boş
    AllocationBlock freeBlock;
    freeBlock.offset = 0;
    freeBlock.size = size;
    freeBlock.isFree = true;
    pool.blocks.push_back(freeBlock);
    
    // Doğru listeye ekle
    std::vector<BufferPool>* targetList = nullptr;
    switch (type)
    {
    case BufferType::BUFFER_VERTEX:   targetList = &m_VertexPools;   break;
    case BufferType::BUFFER_INDEX:    targetList = &m_IndexPools;    break;
    case BufferType::BUFFER_CONSTANT: targetList = &m_ConstantPools; break;
    case BufferType::BUFFER_INSTANCE: targetList = &m_InstancePools; break;
    }
    
    if (targetList)
    {
        targetList->push_back(pool);
        return &targetList->back();
    }
    
    return nullptr;
}

BufferPool* RSD3D11MemoryManager::findPoolWithSpace(BufferType type, size_t requiredSize)
{
    std::vector<BufferPool>* pools = nullptr;
    size_t defaultSize = 0;
    
    switch (type)
    {
    case BufferType::BUFFER_VERTEX:
        pools = &m_VertexPools;
        defaultSize = DEFAULT_VERTEX_POOL_SIZE;
        break;
    case BufferType::BUFFER_INDEX:
        pools = &m_IndexPools;
        defaultSize = DEFAULT_INDEX_POOL_SIZE;
        break;
    case BufferType::BUFFER_CONSTANT:
        pools = &m_ConstantPools;
        defaultSize = DEFAULT_CONSTANT_POOL_SIZE;
        break;
    case BufferType::BUFFER_INSTANCE:
        pools = &m_InstancePools;
        defaultSize = DEFAULT_INSTANCE_POOL_SIZE;
        break;
    }
    
    if (!pools)
        return nullptr;
    
    // Mevcut pool'larda boş yer ara
    for (auto& pool : *pools)
    {
        size_t offset;
        if (findFreeBlock(&pool, requiredSize, offset))
            return &pool;
    }
    
    // Yeterli alan yok, yeni pool oluştur
    size_t newPoolSize = (std::max)(defaultSize, requiredSize * 2);
    return createPool(type, newPoolSize);
}

// ==================== BLOCK MANAGEMENT ====================
bool RSD3D11MemoryManager::findFreeBlock(BufferPool* pPool, size_t requiredSize, size_t& outOffset)
{
    for (const auto& block : pPool->blocks)
    {
        if (block.isFree && block.size >= requiredSize)
        {
            outOffset = block.offset;
            return true;
        }
    }
    return false;
}

void RSD3D11MemoryManager::markBlockUsed(BufferPool* pPool, size_t offset, size_t size)
{
    for (size_t i = 0; i < pPool->blocks.size(); ++i)
    {
        auto& block = pPool->blocks[i];
        if (block.isFree && block.offset == offset && block.size >= size)
        {
            if (block.size == size)
            {
                // Tam eşleşme
                block.isFree = false;
            }
            else
            {
                // Bloğu ikiye böl
                AllocationBlock usedBlock;
                usedBlock.offset = offset;
                usedBlock.size = size;
                usedBlock.isFree = false;
                
                // Kalan boş alan
                block.offset = offset + size;
                block.size -= size;
                
                pPool->blocks.insert(pPool->blocks.begin() + i, usedBlock);
            }
            
            pPool->usedSize += size;
            return;
        }
    }
}

void RSD3D11MemoryManager::markBlockFree(BufferPool* pPool, size_t offset, size_t size)
{
    for (auto& block : pPool->blocks)
    {
        if (!block.isFree && block.offset == offset && block.size == size)
        {
            block.isFree = true;
            pPool->usedSize -= size;
            mergeAdjacentFreeBlocks(pPool);
            return;
        }
    }
}

void RSD3D11MemoryManager::mergeAdjacentFreeBlocks(BufferPool* pPool)
{
    // Offset'e göre sırala
    std::sort(pPool->blocks.begin(), pPool->blocks.end(),
        [](const AllocationBlock& a, const AllocationBlock& b) {
            return a.offset < b.offset;
        });
    
    // Ardışık boş blokları birleştir
    for (size_t i = 0; i < pPool->blocks.size() - 1; )
    {
        auto& current = pPool->blocks[i];
        auto& next = pPool->blocks[i + 1];
        
        if (current.isFree && next.isFree && 
            current.offset + current.size == next.offset)
        {
            current.size += next.size;
            pPool->blocks.erase(pPool->blocks.begin() + i + 1);
        }
        else
        {
            ++i;
        }
    }
}

size_t RSD3D11MemoryManager::alignSize(size_t size, size_t alignment)
{
    return (size + alignment - 1) & ~(alignment - 1);
}

void RSD3D11MemoryManager::releasePool(BufferPool& pool)
{
    if (pool.pBuffer)
    {
        pool.pBuffer->Release();
        pool.pBuffer = nullptr;
    }
    pool.blocks.clear();
}

// ==================== ALLOCATION API ====================
BufferAllocation RSD3D11MemoryManager::allocate(BufferType type, size_t size)
{
    BufferAllocation allocation;
    
    // Constant buffer'lar 16-byte aligned olmalı
    size_t alignedSize = size;
    if (type == BufferType::BUFFER_CONSTANT)
        alignedSize = alignSize(size, 16);
    
    BufferPool* pool = findPoolWithSpace(type, alignedSize);
    if (!pool)
    {
        std::cerr << "[RSD3D11MemoryManager] Failed to find/create pool.\n";
        return allocation;
    }
    
    size_t offset;
    if (!findFreeBlock(pool, alignedSize, offset))
    {
        std::cerr << "[RSD3D11MemoryManager] No free block found.\n";
        return allocation;
    }
    
    markBlockUsed(pool, offset, alignedSize);
    
    allocation.pPool = pool;
    allocation.offset = offset;
    allocation.size = alignedSize;
    allocation.isValid = true;
    
    m_Stats.allocationCount++;
    
    return allocation;
}

void RSD3D11MemoryManager::deallocate(BufferAllocation& allocation)
{
    if (!allocation.isValid || !allocation.pPool)
        return;
    
    markBlockFree(allocation.pPool, allocation.offset, allocation.size);
    
    allocation.pPool = nullptr;
    allocation.offset = 0;
    allocation.size = 0;
    allocation.isValid = false;
    
    if (m_Stats.allocationCount > 0)
        m_Stats.allocationCount--;
}

// ==================== STANDALONE BUFFERS ====================
ID3D11Buffer* RSD3D11MemoryManager::createStandaloneBuffer(BufferType type, size_t size,
                                                            D3D11_USAGE usage,
                                                            const void* initialData)
{
    if (!m_pDevice)
        return nullptr;
    
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = static_cast<UINT>(size);
    desc.Usage = usage;
    
    switch (type)
    {
    case BufferType::BUFFER_VERTEX:
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        break;
    case BufferType::BUFFER_INDEX:
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        break;
    case BufferType::BUFFER_CONSTANT:
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        break;
    case BufferType::BUFFER_INSTANCE:
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        break;
    }
    
    if (usage == D3D11_USAGE_DYNAMIC)
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    D3D11_SUBRESOURCE_DATA* pInitData = nullptr;
    D3D11_SUBRESOURCE_DATA initData = {};
    if (initialData)
    {
        initData.pSysMem = initialData;
        pInitData = &initData;
    }
    
    ID3D11Buffer* buffer = nullptr;
    HRESULT hr = m_pDevice->CreateBuffer(&desc, pInitData, &buffer);
    if (FAILED(hr))
    {
        std::cerr << "[RSD3D11MemoryManager] Failed to create standalone buffer.\n";
        return nullptr;
    }
    
    m_Stats.totalAllocated += size;
    return buffer;
}

void RSD3D11MemoryManager::destroyStandaloneBuffer(ID3D11Buffer* buffer)
{
    if (buffer)
        buffer->Release();
}

// ==================== DATA UPDATE ====================
bool RSD3D11MemoryManager::updateAllocation(const BufferAllocation& allocation, 
                                             const void* data, size_t size)
{
    if (!allocation.isValid || !allocation.pPool || !allocation.pPool->pBuffer)
        return false;
    
    if (size > allocation.size)
    {
        std::cerr << "[RSD3D11MemoryManager] Update size exceeds allocation.\n";
        return false;
    }
    
    D3D11_BOX box = {};
    box.left = static_cast<UINT>(allocation.offset);
    box.right = static_cast<UINT>(allocation.offset + size);
    box.top = 0;
    box.bottom = 1;
    box.front = 0;
    box.back = 1;
    
    m_pContext->UpdateSubresource(allocation.pPool->pBuffer, 0, &box, data, 0, 0);
    return true;
}

// ==================== MAINTENANCE ====================
void RSD3D11MemoryManager::defragment()
{
    // Sadece boş blokları birleştir (gerçek defragmentation CPU-GPU copy gerektirir)
    for (auto& pool : m_VertexPools)
        mergeAdjacentFreeBlocks(&pool);
    for (auto& pool : m_IndexPools)
        mergeAdjacentFreeBlocks(&pool);
    for (auto& pool : m_ConstantPools)
        mergeAdjacentFreeBlocks(&pool);
    for (auto& pool : m_InstancePools)
        mergeAdjacentFreeBlocks(&pool);
}

void RSD3D11MemoryManager::trimUnusedPools()
{
    auto trimList = [this](std::vector<BufferPool>& pools) {
        size_t poolCount = pools.size();
        pools.erase(
            std::remove_if(pools.begin(), pools.end(),
                [this, poolCount](BufferPool& pool) {
                    if (pool.usedSize == 0 && poolCount > 1)
                    {
                        releasePool(pool);
                        return true;
                    }
                    return false;
                }),
            pools.end());
    };
    
    trimList(m_VertexPools);
    trimList(m_IndexPools);
    trimList(m_ConstantPools);
    trimList(m_InstancePools);
}

// ==================== STATS ====================
void RSD3D11MemoryManager::resetStats()
{
    m_Stats = {};
}

void RSD3D11MemoryManager::updateStats()
{
    m_Stats.totalAllocated = 0;
    m_Stats.totalUsed = 0;
    m_Stats.totalFree = 0;
    m_Stats.poolCount = 0;
    m_Stats.fragmentedBlocks = 0;
    
    auto countPool = [this](const std::vector<BufferPool>& pools) {
        for (const auto& pool : pools)
        {
            m_Stats.totalAllocated += pool.totalSize;
            m_Stats.totalUsed += pool.usedSize;
            m_Stats.totalFree += (pool.totalSize - pool.usedSize);
            m_Stats.poolCount++;
            
            // Fragmentation count
            UINT32 freeBlockCount = 0;
            for (const auto& block : pool.blocks)
                if (block.isFree) freeBlockCount++;
            if (freeBlockCount > 1)
                m_Stats.fragmentedBlocks += freeBlockCount - 1;
        }
    };
    
    countPool(m_VertexPools);
    countPool(m_IndexPools);
    countPool(m_ConstantPools);
    countPool(m_InstancePools);
}

void RSD3D11MemoryManager::printPoolInfo() const
{
    std::cout << "\n=== GPU Memory Pool Info ===\n";
    std::cout << "Total Pools: " << m_Stats.poolCount << "\n";
    std::cout << "Total Allocated: " << (m_Stats.totalAllocated / 1024 / 1024) << " MB\n";
    std::cout << "Total Used: " << (m_Stats.totalUsed / 1024 / 1024) << " MB\n";
    std::cout << "Total Free: " << (m_Stats.totalFree / 1024 / 1024) << " MB\n";
    std::cout << "Allocations: " << m_Stats.allocationCount << "\n";
    std::cout << "Fragmented Blocks: " << m_Stats.fragmentedBlocks << "\n";
    std::cout << "=============================\n";
}
