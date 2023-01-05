#pragma once

#include <foundation\Common.hpp>
#include <memory\Pointer.hpp>
#include <math\SimpleMath.hpp>

DRE_BEGIN_NAMESPACE

/*
*
* Pool allocation implementation.
* Allocations have fixed size.
* Basically, free-list of aligned equal memory chunks
*
*
* Basic interface:
*
*   + Alloc     ()      // allocation of fixed size
*   + Free      (ptr)
*   + Reset     ()
*   + MemorySize()
*
*/
class AllocatorPool
{
public:
    inline void* Alloc()
    {
        if (m_NextFreeChunk == nullptr)
            return nullptr;

        void* result = m_NextFreeChunk;
        m_NextFreeChunk = *(void**)m_NextFreeChunk;
        return result;
    }

    inline void Free(void* memory)
    {
        *(void**)memory = m_NextFreeChunk;
        m_NextFreeChunk = memory;
    }

    inline U64 MemorySize() const
    {
        return m_Size;
    }

    inline void Reset(void* memory, U64 size, U32 poolChunkSize, U32 poolChunkAlignment, U32 firstFreeChunk)
    {
        m_Memory = memory;
        m_Size = size;
        m_PoolChunkSize = poolChunkSize;
        m_PoolChunkAlignment = poolChunkAlignment;
        m_NextFreeChunk = nullptr;

        DRE_ASSERT(m_Memory != nullptr, "AllocatorPool: received null memory.");
        DRE_ASSERT(m_Size != 0, "AllocatorPool: received null size.");
        DRE_ASSERT(IsValidChunkAlignment(), "AllocatorPool: invalid chunk alignment (NPOT).");
        DRE_ASSERT(IsValidChunkSize(), "AllocatorPool: invalid chunk size (SIZE <= sizeof(void*) or not aligned).");

        // prepearing free-list
        void* const startChunk = DRE::PtrAdd(ChunksStart(), firstFreeChunk * m_PoolChunkSize);
        void* chunk = startChunk;

        U32 const count = ChunksCount() - 1;
        for (U32 i = firstFreeChunk; i < count; ++i)
        {
            void* nextChunk = PtrAdd(chunk, m_PoolChunkSize);
            *((void**)chunk) = nextChunk;
            chunk = nextChunk;
        }
        *(void**)chunk = nullptr;
        m_NextFreeChunk = startChunk;
    }



    AllocatorPool()
        : m_Memory{ nullptr }
        , m_Size{ 0 }
        , m_PoolChunkSize{ 0 }
        , m_PoolChunkAlignment{ 0 }
        , m_NextFreeChunk{ nullptr }
    {
    }

    AllocatorPool(void* memory, U64 size, U32 poolChunkSize, U32 poolChunkAlignment)
        : m_Memory{ nullptr }
        , m_Size{ 0 }
        , m_PoolChunkSize{ 0 }
        , m_PoolChunkAlignment{ 0 }
        , m_NextFreeChunk{ nullptr }
    {
        Reset(memory, size, poolChunkSize, poolChunkAlignment, 0);
    }

    AllocatorPool(AllocatorPool&& rhs)
    {
        operator=(DRE_MOVE(rhs));
    }

    AllocatorPool& operator=(AllocatorPool&& rhs)
    {
        DRE_ASSERT(m_Memory == nullptr, "Can't move-assign to initialized allocators.");
        m_Memory = rhs.m_Memory;                            rhs.m_Memory = nullptr;
        m_Size = rhs.m_Size;                                rhs.m_Size = 0;
        m_PoolChunkSize = rhs.m_PoolChunkSize;              rhs.m_PoolChunkSize = 0;
        m_PoolChunkAlignment = rhs.m_PoolChunkAlignment;    rhs.m_PoolChunkAlignment = 0;
        m_NextFreeChunk = rhs.m_NextFreeChunk;              rhs.m_NextFreeChunk = nullptr;

        return *this;
    }

    AllocatorPool(AllocatorPool const& rhs) = delete;
    AllocatorPool& operator=(AllocatorPool const& rhs) = delete;

    ~AllocatorPool()
    {
        m_Memory = nullptr;
        m_Size = 0;
        m_PoolChunkSize = 0;
        m_PoolChunkAlignment = 0;
        m_NextFreeChunk = nullptr;
    }

    inline void* ChunksStart() const
    {
        return PtrAlign(m_Memory, m_PoolChunkAlignment);
    }

    inline U32 ChunksCount() const
    {
        return U32((m_Size - m_PoolChunkAlignment) / m_PoolChunkSize);
    }

private:
    inline bool IsValidChunkAlignment()
    {
        return IsPowOf2(m_PoolChunkAlignment);
    }

    inline bool IsValidChunkSize()
    {
        return (m_PoolChunkSize >= sizeof(void*)) && ((m_PoolChunkSize & (m_PoolChunkAlignment - 1)) == 0);
    }



private:
    // Generic allocator section
    void*       m_Memory;
    U64         m_Size;

    // Pool allocator section
    U32         m_PoolChunkSize;
    U32         m_PoolChunkAlignment;
    void*       m_NextFreeChunk;

};

DRE_END_NAMESPACE

