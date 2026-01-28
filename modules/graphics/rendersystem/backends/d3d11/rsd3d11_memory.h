#pragma once

#include <d3d11.h>
#include <vector>
#include <unordered_map>
#include "../../../../headeronly/globaltypes.h"

// ==================== BUFFER TYPES ====================
enum class BufferType : UINT32
{
    BUFFER_VERTEX,
    BUFFER_INDEX,
    BUFFER_CONSTANT,
    BUFFER_INSTANCE
};

// ==================== ALLOCATION BLOCK ====================
struct AllocationBlock
{
    size_t offset;
    size_t size;
    bool isFree;
};

// ==================== BUFFER POOL ====================
struct BufferPool
{
    ID3D11Buffer* pBuffer;
    size_t totalSize;
    size_t usedSize;
    BufferType type;
    std::vector<AllocationBlock> blocks;
};

// ==================== BUFFER ALLOCATION ====================
struct BufferAllocation
{
    BufferPool* pPool;
    size_t offset;
    size_t size;
    bool isValid;
    
    BufferAllocation() : pPool(nullptr), offset(0), size(0), isValid(false) {}
};

// ==================== MEMORY STATS ====================
struct MemoryStats
{
    size_t totalAllocated;
    size_t totalUsed;
    size_t totalFree;
    UINT32 allocationCount;
    UINT32 poolCount;
    UINT32 fragmentedBlocks;
};

// ==================== D3D11 MEMORY MANAGER ====================
// Unreal Engine tarzı GPU buffer pool yönetimi
// Buffer bind call'larını minimize eder
class RSD3D11MemoryManager
{
private:
    ID3D11Device* m_pDevice;
    ID3D11DeviceContext* m_pContext;
    
    // Pool'lar - buffer tipi başına
    std::vector<BufferPool> m_VertexPools;
    std::vector<BufferPool> m_IndexPools;
    std::vector<BufferPool> m_ConstantPools;
    std::vector<BufferPool> m_InstancePools;
    
    // Default pool boyutları
    static constexpr size_t DEFAULT_VERTEX_POOL_SIZE = 16 * 1024 * 1024;   // 16 MB
    static constexpr size_t DEFAULT_INDEX_POOL_SIZE = 8 * 1024 * 1024;     // 8 MB
    static constexpr size_t DEFAULT_CONSTANT_POOL_SIZE = 1 * 1024 * 1024;  // 1 MB
    static constexpr size_t DEFAULT_INSTANCE_POOL_SIZE = 4 * 1024 * 1024;  // 4 MB
    
    // Stats
    MemoryStats m_Stats;
    
private:
    // Pool oluşturma
    BufferPool* createPool(BufferType type, size_t size);
    
    // Uygun pool bulma
    BufferPool* findPoolWithSpace(BufferType type, size_t requiredSize);
    
    // Block yönetimi
    bool findFreeBlock(BufferPool* pPool, size_t requiredSize, size_t& outOffset);
    void markBlockUsed(BufferPool* pPool, size_t offset, size_t size);
    void markBlockFree(BufferPool* pPool, size_t offset, size_t size);
    void mergeAdjacentFreeBlocks(BufferPool* pPool);
    
    // Alignment hesaplama (D3D11 constant buffer 16-byte alignment gerektirir)
    size_t alignSize(size_t size, size_t alignment);
    
    // Pool temizleme
    void releasePool(BufferPool& pool);

public:
    RSD3D11MemoryManager();
    ~RSD3D11MemoryManager();
    
    // Disable copy/move
    RSD3D11MemoryManager(const RSD3D11MemoryManager&) = delete;
    RSD3D11MemoryManager& operator=(const RSD3D11MemoryManager&) = delete;
    
    // Lifecycle
    bool initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    void shutdown();
    
    // ==================== ALLOCATION API ====================
    // Suballocation from pool (hızlı, bind minimize)
    BufferAllocation allocate(BufferType type, size_t size);
    void deallocate(BufferAllocation& allocation);
    
    // Standalone buffer (büyük veya özel resource'lar için)
    ID3D11Buffer* createStandaloneBuffer(BufferType type, size_t size, 
                                          D3D11_USAGE usage = D3D11_USAGE_DEFAULT,
                                          const void* initialData = nullptr);
    void destroyStandaloneBuffer(ID3D11Buffer* buffer);
    
    // ==================== DATA UPDATE ====================
    // Pool içindeki allocation'a data yaz
    bool updateAllocation(const BufferAllocation& allocation, const void* data, size_t size);
    
    // ==================== MAINTENANCE ====================
    // Defragmentation (frame sonunda çağır)
    void defragment();
    
    // Kullanılmayan pool'ları temizle
    void trimUnusedPools();
    
    // ==================== STATS ====================
    const MemoryStats& getStats() const { return m_Stats; }
    void resetStats();
    void updateStats();
    
    // Debug
    void printPoolInfo() const;
};
