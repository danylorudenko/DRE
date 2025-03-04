#pragma once

#include <foundation\Common.hpp>
#include <foundation\math\SimpleMath.hpp>

DRE_BEGIN_NAMESPACE


////////////////////////////////////////////////
// FreeListElementAllocator
////////////////////////////////////////////////
template<std::uint16_t LIST_SIZE>
class FreeListElementAllocator
{
    static_assert(LIST_SIZE - 1 < DRE_U16_MAX);

    static constexpr std::uint16_t INVALID_ELEMENT = DRE_U16_MAX;

public:
    FreeListElementAllocator()
    {
        Reset();
    }

    FreeListElementAllocator(FreeListElementAllocator&& rhs)
    {
        operator=(std::move(rhs));
    }

    FreeListElementAllocator& operator=(FreeListElementAllocator&& rhs)
    {
        std::memcpy(this, &rhs, sizeof(*this));
        return *this;
    }

    std::uint16_t Allocate()
    {
        DRE_ASSERT(firstFree_ != INVALID_ELEMENT, "FreeListElementAllocator has no free entries.");

        std::uint16_t result = firstFree_;
        firstFree_ = storage_[firstFree_];
        return result;
    }

    void Free(std::uint16_t element)
    {
        storage_[element] = firstFree_;
        firstFree_ = element;
    }

    void Reset()
    {
        for (std::uint16_t i = 0; i < LIST_SIZE - 1; i++)
        {
            storage_[i] = i + 1;
        }
        storage_[LIST_SIZE - 1] = INVALID_ELEMENT;

        firstFree_ = 0;
    }

private:
    std::uint16_t storage_[LIST_SIZE];
    std::uint16_t firstFree_;
};


////////////////////////////////////////////////
// LinearElementAllocator
////////////////////////////////////////////////
template<std::uint16_t SIZE>
class LinearElementAllocator
{
public:
    std::uint16_t Allocate(std::uint16_t count)
    {
        DRE_ASSERT((nextFree_ + count) < SIZE, "LinearElementAllocator has no free space left.");

        std::uint16_t result = nextFree_;
        nextFree_ += count;
        return result;
    }

    void Free(std::uint16_t element) {}

    void Reset()
    {
        nextFree_ = 0;
    }

private:
    std::uint16_t nextFree_;
};

template<std::uint16_t SIZE>
class DummyElementsAllocator
{
    static constexpr std::uint16_t INVALID_ELEMENT = DRE_U16_MAX;
public:
    std::uint16_t Allocate  (std::uint16_t count) { /* noop */ return INVALID_ELEMENT; }
    void Free               (std::uint16_t element) { /* noop */ }
    void Reset              () { /* noop */ }
};




/*
*
* BuddyElementAllocator.
*
* Basic interface:
*
*   + Alloc     (size)
*   + Free      ()
*   + MemorySize()
*   + Reset     ()
*
*   + RequiredMemorySize() <-- this returns required memory size on compile time
*
*
*
*
* Algorithm:
*
*  1  get_target_depth()
*  2  if(no free chunk on target_depth)
*  3      go to higher depth
*  4      try split chunk higher
*  5      go to 2
*  6  else
*  7      get free chunk on target_depth
*
*
* Runtime scheme:
*                                 _______________________________
*   *freelist_ptr[0]-->null      |ssssssssssssss_0_ssssssssssssss|
*   *freelist_ptr[1]------------>|_______1_______|ssssss_2_ssssss| <- split
*   *freelist_ptr[2]-----        |___3___|___4___|XX_5_XX|___6___|
*                       |                                    ^
*                       |____________________________________|
*
* Here user requrested small chunk.
* Initially, only the biggest block is considered free.
* 1 - try get chunk on lowest depth
* 2 - there is no block, try get free chunk higher and split it
* 3 - there is no block, try to get free chunk on highest level, there is a root chunk, split it and return recursively
*
*/
template<U64 LEAF_SIZE, U8 MAX_DEPTH>
class BuddyElementAllocator
{
public:
    static constexpr U64 INVALID_ELEMENT = DRE_U64_MAX;


#ifdef DRE_DEBUG
    inline void PrintIsFreeState()
    {
        int globalIndex = 0;
        for (int i = 0; i < MAX_DEPTH + 1; i++)
        {
            printf("level%i:", i);
            for (int ii = 0; ii < GetChunksCountOnDepth(i); ii++)
            {
                printf("%d", MetaIsChunkFreeByGlobalIndex(globalIndex));
                globalIndex++;
            }
            printf("\n");
        }

    }
#endif

    // WARNING: alignment param is unused, all allocations are 256-aligned
    inline U64 Alloc(U64 size, U32 alignment)
    {
        DRE_ASSERT(size < DRE_U32_MAX, "BuddyElementAllocator currently supports allocations only < DRE_U32_MAX");

        U64 const potSize = (U64)NextPowOf2((U32)size);
         U8 const depth = GetDepthBySize(potSize);
        if (size > RootChunkSize())
            return INVALID_ELEMENT;

        U64 result = RecursiveAllocInternal(depth);
        if (result != INVALID_ELEMENT)
        {
            U32 const globalIndex = GetGlobalStateIndex(result, depth);
            MetaSetChunkUsedByGlobalIndex(globalIndex);
        }
        return result;
    }

    inline void Free(U64 element)
    {
        U8 const depth = MetaGetChunkDepth(element);

        U32 const globalIndex = GetGlobalStateIndex(element, depth);
        MetaSetChunkFreeByGlobalIndex(globalIndex);

        MetaPutFreeChunkOnDepth(depth, element);

        RecursiveMergeInternal(depth, element);
    }


private:
    U64 RecursiveAllocInternal(U8 depth)
    {
        U64 freeChunkOffset = INVALID_ELEMENT;
        MetaChunkHeader* freeChunk = MetaExtractFirstFreeChunkOnDepth(depth);
        if (freeChunk != nullptr)
        {
            return freeChunk->elementOffset;
        }
        else
        {
            if (depth == 0)
                return INVALID_ELEMENT;

            freeChunkOffset = RecursiveAllocInternal(depth - 1);
            if (freeChunkOffset != INVALID_ELEMENT)
            {
                // here we split chunk on *depth*, put two children on free list on (depth)
                SplitChunkOnDepth(freeChunkOffset, depth - 1);
            }
            else
            {
                return INVALID_ELEMENT;
            }
        }

        return MetaExtractFirstFreeChunkOnDepth(depth)->elementOffset;
    }

    void SplitChunkOnDepth(U64 chunk, U8 depth)
    {
        U8 const childDepth = depth + 1;
        U64 const childSize = ChunkSizeByDepth(childDepth);
        U64 left = chunk;
        U64 right = chunk + childSize;

        MetaSetChunkDepth(left, childDepth);
        MetaSetChunkDepth(right, childDepth);

        U32 const globalId = GetGlobalStateIndex(chunk, depth);
        MetaSetChunkUsedByGlobalIndex(globalId);

        MetaPutFreeChunkOnDepth(childDepth, left);
        MetaPutFreeChunkOnDepth(childDepth, right);

        MetaSetChunkFreeByGlobalIndex(GetGlobalStateIndex(left, childDepth));
        MetaSetChunkFreeByGlobalIndex(GetGlobalStateIndex(right, childDepth));
    }


    void RecursiveMergeInternal(U8 depth, U64 element)
    {
        if (depth == 0)
            return;

        U32 const index = GetIndexInDepth(element, depth);
        U64 const size = GetChunkSize(element);

        DRE_ASSERT(MetaIsChunkFreeOnDepth(element, depth), "The merging element is not free on merging depth!");

        if (index % 2 == 0) // uneven
        {
            // buddy is on the right
            U64 buddy = element + size;
            if (MetaIsChunkFreeOnDepth(buddy, depth)) // merge
            {
                DRE_ASSERT(MetaGetChunkDepth(element) == MetaGetChunkDepth(buddy), "The merging element has incorrect depth!");

                MetaRemoveFreeChunkOnDepth(depth, element);
                MetaRemoveFreeChunkOnDepth(depth, buddy);

                MetaPutFreeChunkOnDepth(depth - 1, element);

                DRE_DEBUG_ONLY(MetaSetChunkDepth(buddy, 255));
                MetaSetChunkDepth(element, depth - 1);

                U32 const globalIndex = GetGlobalStateIndex(element, depth - 1);
                MetaSetChunkFreeByGlobalIndex(globalIndex);

                RecursiveMergeInternal(depth - 1, element);
            }
        }
        else // even
        {
            // buddy is on the left
            U64 buddy = element - size;
            if (MetaIsChunkFreeOnDepth(buddy, depth))
            {
                DRE_ASSERT(MetaGetChunkDepth(element) == MetaGetChunkDepth(buddy), "The merging element has incorrect depth!");

                MetaRemoveFreeChunkOnDepth(depth, element);
                MetaRemoveFreeChunkOnDepth(depth, buddy);

                MetaPutFreeChunkOnDepth(depth - 1, buddy);

                DRE_DEBUG_ONLY(MetaSetChunkDepth(element, 255));
                MetaSetChunkDepth(buddy, depth - 1);

                U32 const globalIndex = GetGlobalStateIndex(buddy, depth - 1);
                MetaSetChunkFreeByGlobalIndex(globalIndex);

                RecursiveMergeInternal(depth - 1, buddy);
            }
        }
    }


public:
    inline void Reset()
    {
        MetaPutFreeChunkOnDepth(0, 0);
        for (U32 i = 1; i < (U32)(MaxDepth() + 1); ++i)
        {
            m_MetaData.depthFreeLists[i] = nullptr;
        }

        MetaSetChunkDepth(0, 0);
        MetaSetChunkFreeByGlobalIndex(0);
    }


public:
    BuddyElementAllocator()
        : m_MetaData{}
    {
        Reset();
    }

    BuddyElementAllocator(BuddyElementAllocator&& rhs)
        : m_MetaData{}
    {
        operator=(DRE_MOVE(rhs));
    }

    BuddyElementAllocator& operator=(BuddyElementAllocator&& rhs)
    {
        m_MetaData = rhs.m_MetaData;        rhs.m_MetaData = MetaData{};

        return *this;
    }

    BuddyElementAllocator(BuddyElementAllocator const&) = delete;
    BuddyElementAllocator& operator=(BuddyElementAllocator const& rhs) = delete;

    ~BuddyElementAllocator()
    {
        m_MetaData = MetaData();
    }




// chunk management
public:
    static constexpr bool IsValidRootChunkSize()
    {
        return IsPowOf2(RootChunkSize());
    }

    static constexpr U8 MaxDepth()
    {
        return MAX_DEPTH;
    }

    static constexpr U32 LeafSize()
    {
        return LEAF_SIZE;
    }

    static constexpr U32 LeavesCount()
    {
        return 1U << (MAX_DEPTH);
    }

    static constexpr U32 AllPossibleChunksCount()
    {
        return (1U << (MAX_DEPTH + 1)) - 1;
    }

    static constexpr U64 RootChunkSize()
    {
        return (LeavesCount()) * LEAF_SIZE;
    }

    static constexpr U32 RootChunkAlignment()
    {
        return 256;
    }

    static constexpr U64 ChunkSizeByDepth(U8 depth)
    {
        return RootChunkSize() >> depth;
    }

    static constexpr U8 GetDepthBySize(U64 size)
    {
        DRE_ASSERT(IsPowOf2(size), "Argument must be PowOf2.");

        U64 const rootPow = Log2(RootChunkSize());
        U64 const pow = Log2(size);

        DRE_ASSERT(pow <= rootPow, "Requested too big chunk");

        return (U8)Min<U64>(MaxDepth(), rootPow - pow);
    }

    static constexpr U64 GetChunksCountOnDepth(U8 depth)
    {
        return RootChunkSize() / ChunkSizeByDepth(depth);
    }

    // returns index on which states of the chunks on *depth* starts
    static constexpr U32 GetChunkStateDepthStart(U8 depth)
    {
        return (1U << depth) - 1;
    }

private:
    inline U64 GetChunkSize(U64 chunk)
    {
        return ChunkSizeByDepth(MetaGetChunkDepth(chunk));
    }

    inline U32 GetIndexInDepth(U64 chunk, U8 depth)
    {
        return (U32)(chunk) / ChunkSizeByDepth(depth);
    }

    inline U32 GetGlobalStateIndex(U64 chunk, U8 depth)
    {
        U32 const depthGlobalIndex = GetChunkStateDepthStart(depth);
        U32 const indexInDepth = GetIndexInDepth(chunk, depth);
        U32 const result = depthGlobalIndex + indexInDepth;
        DRE_ASSERT(result < AllPossibleChunksCount(), "Invalid global index");
        return result;
    }

    static_assert(IsValidRootChunkSize(), "BuddyElementAllocator: root chunk is not POT.");



    // metadata
private:
    struct MetaChunkHeader
    {
        MetaChunkHeader* prev;
        MetaChunkHeader* next;
        U64 elementOffset;
    };

    struct MetaData
    {
        // freelists for chunks on all depths
        MetaChunkHeader* depthFreeLists[MaxDepth() + 1];

        MetaChunkHeader chunkHeaderStorage[AllPossibleChunksCount()];

        // in 1 bits we will store states of the chunks on all levels: Allocated/Free
        // 0 - free, 1 - used
        U8 chunksStates[std::max(1u, AllPossibleChunksCount() / 8 + 1)];

        // for all possible locations buddy can return
        U8 chunksDepth[LeavesCount()];
    };

    inline bool MetaIsChunkFreeOnDepth(U64 chunk, U8 depth)
    {
        U32 const stateGlobalIndex = GetGlobalStateIndex(chunk, depth);
        return MetaIsChunkFreeByGlobalIndex(stateGlobalIndex);
    }

    inline bool MetaIsChunkFreeByGlobalIndex(U32 globalIndex)
    {
        return ((m_MetaData.chunksStates[globalIndex / 8]) & (1 << (globalIndex % 8))) == 0;
    }

    inline void MetaSetChunkUsedByGlobalIndex(U32 globalIndex)
    {
        m_MetaData.chunksStates[globalIndex / 8] |= (1 << (globalIndex % 8));
    }

    inline void MetaSetChunkFreeByGlobalIndex(U32 globalIndex)
    {
        m_MetaData.chunksStates[globalIndex / 8] &= (~(1 << (globalIndex % 8)));
    }

    inline U8 MetaGetChunkDepth(U64 chunk)
    {
        DRE_DEBUG_ONLY(DRE_ASSERT(m_MetaData.chunksDepth[chunk / LeafSize()] != 255, "Junk depth in MetaData!"));

        return m_MetaData.chunksDepth[chunk / LeafSize()];
    }

    inline void MetaSetChunkDepth(U64 chunk, U8 depth)
    {
#ifdef DRE_DEBUG
        U32 leavesPerChunk = ChunkSizeByDepth(depth) / LEAF_SIZE;
        for (U32 i = 1; i < leavesPerChunk; i++)
        {
            MetaSetChunkDepth(chunk + i * LEAF_SIZE, 255);
        }
#endif
        DRE_ASSERT(depth <= MAX_DEPTH || depth == 255, "Attempt to store invalid depth.");

        m_MetaData.chunksDepth[chunk / LeafSize()] = depth;
    }

    inline typename BuddyElementAllocator<LEAF_SIZE, MAX_DEPTH>::MetaChunkHeader* MetaExtractFirstFreeChunkOnDepth(U8 depth)
    {
        if (m_MetaData.depthFreeLists[depth] != nullptr)
        {
            DRE_ASSERT(m_MetaData.depthFreeLists[depth]->prev == nullptr, "First header must have prev == nullptr");

            MetaChunkHeader* result = m_MetaData.depthFreeLists[depth];
            if (result->next != nullptr)
            {
                DRE_ASSERT(result->next->prev == result, "Discrepancy in double-linked list. Prev and Next don't link each other.");
                result->next->prev = nullptr; // no prev because it's first
            }
            m_MetaData.depthFreeLists[depth] = result->next;
            return result;
        }
        else
        {
            return nullptr;
        }
    }

    inline void MetaRemoveFreeChunkOnDepth(U8 depth, U64 element)
    {
        DRE_ASSERT(m_MetaData.depthFreeLists[depth]->prev == nullptr, "First header must have prev == nullptr");

        if (element == m_MetaData.depthFreeLists[depth]->elementOffset)
        {
            if (m_MetaData.depthFreeLists[depth]->next != nullptr)
                DRE_ASSERT(m_MetaData.depthFreeLists[depth] == m_MetaData.depthFreeLists[depth]->next->prev, "Discrepancy in double-linked list. Prev and Next don't link each other.");

            m_MetaData.depthFreeLists[depth] = m_MetaData.depthFreeLists[depth]->next;
            if (m_MetaData.depthFreeLists[depth] != nullptr)
                m_MetaData.depthFreeLists[depth]->prev = nullptr;
            return;
        }

        MetaChunkHeader* header = m_MetaData.chunkHeaderStorage + GetGlobalStateIndex(element, depth);
        MetaChunkHeader* prev = header->prev;
        MetaChunkHeader* next = header->next;

        DRE_ASSERT(prev != nullptr, "Prev header must exist");

        prev->next = next;

        if (next)
            next->prev = prev;
    }

    inline void MetaPutFreeChunkOnDepth(U8 depth, U64 element)
    {
        MetaChunkHeader* lastFree = m_MetaData.depthFreeLists[depth];
        MetaChunkHeader* newFree = m_MetaData.chunkHeaderStorage + GetGlobalStateIndex(element, depth);

        newFree->prev = nullptr;
        newFree->next = m_MetaData.depthFreeLists[depth];
        newFree->elementOffset = element;

        if (m_MetaData.depthFreeLists[depth] != nullptr)
        {
            lastFree->prev = newFree;
        }

        m_MetaData.depthFreeLists[depth] = newFree;
    }

    MetaData m_MetaData;
};


DRE_END_NAMESPACE
