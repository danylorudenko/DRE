#pragma once

#include <foundation\Common.hpp>
#include <foundation\math\SimpleMath.hpp>
#include <foundation\memory\Pointer.hpp>
#include <foundation\memory\MemoryOps.hpp>

DRE_BEGIN_NAMESPACE

/*
*
* AllocatorBuddyBase.
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
template<U64 LEAF_SIZE, U8 MAX_DEPTH, typename TDerived>
class AllocatorBuddyBase
{
public:
    static constexpr U64 INVALID_OFFSET = DRE_U64_MAX;


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
        DRE_ASSERT(size < DRE_U32_MAX, "AllocatorBuddyBase currently supports allocations only < DRE_U32_MAX");

        U64 const potSize = (U64)NextPowOf2((U32)size);
        U8 const depth = GetDepthBySize(potSize);
        if (size > RootChunkSize())
            return INVALID_OFFSET;

        U64 result = RecursiveAllocInternal(depth);
        if (result != INVALID_OFFSET)
        {
            U32 const globalIndex = GetGlobalStateIndex(result, depth);
            DRE_ASSERT(MetaIsChunkFreeByGlobalIndex(globalIndex), "Allocated chunk must be free!");
            MetaSetChunkUsedByGlobalIndex(globalIndex);
        }
        return result;
    }

    inline void Free(U64 chunkOffset)
    {
        U8 const depth = MetaGetChunkDepth(chunkOffset);

        U32 const globalIndex = GetGlobalStateIndex(chunkOffset, depth);
        MetaSetChunkFreeByGlobalIndex(globalIndex);

        MetaPutFreeChunkOnDepth(depth, chunkOffset);

        RecursiveMergeInternal(depth, chunkOffset);
    }


private:
    U64 RecursiveAllocInternal(U8 depth)
    {
        U64 freeChunkOffset = INVALID_OFFSET;
        MetaChunkHeader* freeChunk = MetaExtractFirstFreeChunkOnDepth(depth);
        if (freeChunk != nullptr)
        {
            return freeChunk->chunkOffset;
        }
        else
        {
            if (depth == 0)
                return INVALID_OFFSET;

            freeChunkOffset = RecursiveAllocInternal(depth - 1);
            if (freeChunkOffset != INVALID_OFFSET)
            {
                // here we split chunk on *depth*, put two children on free list on (depth)
                SplitChunkOnDepth(freeChunkOffset, depth - 1);
            }
            else
            {
                return INVALID_OFFSET;
            }
        }

        return MetaExtractFirstFreeChunkOnDepth(depth)->chunkOffset;
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


    void RecursiveMergeInternal(U8 depth, U64 chunkOffset)
    {
        if (depth == 0)
            return;

        U32 const index = GetIndexInDepth(chunkOffset, depth);
        U64 const size = GetChunkSize(chunkOffset);

        DRE_ASSERT(MetaIsChunkFreeOnDepth(chunkOffset, depth), "The merging chunkOffset is not free on merging depth!");

        if (index % 2 == 0) // uneven
        {
            // buddy is on the right
            U64 buddy = chunkOffset + size;
            if (MetaIsChunkFreeOnDepth(buddy, depth)) // merge
            {
                DRE_ASSERT(MetaGetChunkDepth(chunkOffset) == MetaGetChunkDepth(buddy), "The merging chunkOffset has incorrect depth!");

                MetaRemoveFreeChunkOnDepth(depth, chunkOffset);
                MetaRemoveFreeChunkOnDepth(depth, buddy);

                MetaPutFreeChunkOnDepth(depth - 1, chunkOffset);

                DRE_DEBUG_ONLY(MetaSetChunkDepth(buddy, 255));
                MetaSetChunkDepth(chunkOffset, depth - 1);

                U32 const globalIndex = GetGlobalStateIndex(chunkOffset, depth - 1);
                MetaSetChunkFreeByGlobalIndex(globalIndex);

                RecursiveMergeInternal(depth - 1, chunkOffset);
            }
        }
        else // even
        {
            // buddy is on the left
            U64 buddy = chunkOffset - size;
            if (MetaIsChunkFreeOnDepth(buddy, depth))
            {
                DRE_ASSERT(MetaGetChunkDepth(chunkOffset) == MetaGetChunkDepth(buddy), "The merging chunkOffset has incorrect depth!");

                MetaRemoveFreeChunkOnDepth(depth, chunkOffset);
                MetaRemoveFreeChunkOnDepth(depth, buddy);

                MetaPutFreeChunkOnDepth(depth - 1, buddy);

                DRE_DEBUG_ONLY(MetaSetChunkDepth(chunkOffset, 255));
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
            GetMetaData().depthFreeLists[i] = nullptr;
        }

        MetaSetChunkDepth(0, 0);
        MetaSetChunkFreeByGlobalIndex(0);
    }


public:
    AllocatorBuddyBase()
    {
    }

    AllocatorBuddyBase(AllocatorBuddyBase&& rhs)
    {
        operator=(DRE_MOVE(rhs));
    }

    AllocatorBuddyBase& operator=(AllocatorBuddyBase&& rhs)
    {
        return *this;
    }

    AllocatorBuddyBase(AllocatorBuddyBase const&) = delete;
    AllocatorBuddyBase& operator=(AllocatorBuddyBase const& rhs) = delete;

    virtual ~AllocatorBuddyBase()
    {
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

protected:
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

    static_assert(IsValidRootChunkSize(), "AllocatorBuddyBase: root chunk is not POT.");



    // metadata
protected:
    struct MetaChunkHeader
    {
        MetaChunkHeader* prev;
        MetaChunkHeader* next;
        U64 chunkOffset;
    };

    struct MetaData
    {
        // freelists for chunks on all depths
        MetaChunkHeader* depthFreeLists[MaxDepth() + 1];

        // in 1 bits we will store states of the chunks on all levels: Allocated/Free
        // 0 - free, 1 - used
        U8 chunksStates[std::max(1u, AllPossibleChunksCount() / 8 + 1)];

        // for all possible locations buddy can return
        U8 chunksDepth[LeavesCount()];
    };

    MetaData& GetMetaData()
    {
        return static_cast<TDerived*>(this)->GetMetaDataImpl();
    }

    MetaChunkHeader* GetChunkHeader(U64 chunkOffset)
    {
        return static_cast<TDerived*>(this)->GetChunkHeaderImpl(chunkOffset);
    }

    inline bool MetaIsChunkFreeOnDepth(U64 chunkOffset, U8 depth)
    {
        U32 const stateGlobalIndex = GetGlobalStateIndex(chunkOffset, depth);
        return MetaIsChunkFreeByGlobalIndex(stateGlobalIndex);
    }

    inline bool MetaIsChunkFreeByGlobalIndex(U32 globalIndex)
    {
        return ((GetMetaData().chunksStates[globalIndex / 8]) & (1 << (globalIndex % 8))) == 0;
    }

    inline void MetaSetChunkUsedByGlobalIndex(U32 globalIndex)
    {
        GetMetaData().chunksStates[globalIndex / 8] |= (1 << (globalIndex % 8));
    }

    inline void MetaSetChunkFreeByGlobalIndex(U32 globalIndex)
    {
        GetMetaData().chunksStates[globalIndex / 8] &= (~(1 << (globalIndex % 8)));
    }

    inline U8 MetaGetChunkDepth(U64 chunk)
    {
        DRE_DEBUG_ONLY(DRE_ASSERT(GetMetaData().chunksDepth[chunk / LeafSize()] != 255, "Junk depth in MetaData!"));

        return GetMetaData().chunksDepth[chunk / LeafSize()];
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

        GetMetaData().chunksDepth[chunk / LeafSize()] = depth;
    }

    inline typename AllocatorBuddyBase<LEAF_SIZE, MAX_DEPTH, TDerived>::MetaChunkHeader* MetaExtractFirstFreeChunkOnDepth(U8 depth)
    {
        if (GetMetaData().depthFreeLists[depth] != nullptr)
        {
            DRE_ASSERT(GetMetaData().depthFreeLists[depth]->prev == nullptr, "First header must have prev == nullptr");

            MetaChunkHeader* result = GetMetaData().depthFreeLists[depth];
            if (result->next != nullptr)
            {
                DRE_ASSERT(result->next->prev == result, "Discrepancy in double-linked list. Prev and Next don't link each other.");
                result->next->prev = nullptr; // no prev because it's first
            }
            GetMetaData().depthFreeLists[depth] = result->next;
            return result;
        }
        else
        {
            return nullptr;
        }
    }

    inline void MetaRemoveFreeChunkOnDepth(U8 depth, U64 chunkOffset)
    {
        DRE_ASSERT(GetMetaData().depthFreeLists[depth]->prev == nullptr, "First header must have prev == nullptr");

        if (chunkOffset == GetMetaData().depthFreeLists[depth]->chunkOffset)
        {
            if (GetMetaData().depthFreeLists[depth]->next != nullptr)
                DRE_ASSERT(GetMetaData().depthFreeLists[depth] == GetMetaData().depthFreeLists[depth]->next->prev, "Discrepancy in double-linked list. Prev and Next don't link each other.");

            GetMetaData().depthFreeLists[depth] = GetMetaData().depthFreeLists[depth]->next;
            if (GetMetaData().depthFreeLists[depth] != nullptr)
                GetMetaData().depthFreeLists[depth]->prev = nullptr;
            return;
        }

        //MetaChunkHeader* header = m_MetaData->chunkHeaderStorage + GetGlobalStateIndex(chunkOffset, depth);
        MetaChunkHeader* header = GetChunkHeader(chunkOffset);
        MetaChunkHeader* prev = header->prev;
        MetaChunkHeader* next = header->next;

        DRE_ASSERT(prev != nullptr, "Prev header must exist");

        prev->next = next;

        if (next)
            next->prev = prev;
    }

    inline void MetaPutFreeChunkOnDepth(U8 depth, U64 chunkOffset)
    {
        MetaChunkHeader* lastFree = GetMetaData().depthFreeLists[depth];
        MetaChunkHeader* newFree = GetChunkHeader(chunkOffset);

        newFree->prev = nullptr;
        newFree->next = GetMetaData().depthFreeLists[depth];
        newFree->chunkOffset = chunkOffset;

        if (GetMetaData().depthFreeLists[depth] != nullptr)
        {
            lastFree->prev = newFree;
        }

        GetMetaData().depthFreeLists[depth] = newFree;
    }
};


////////////////////////////////////////////////
// AllocatorBuddy specialization of AllocatorBuddyOffset
////////////////////////////////////////////////
template<U64 LEAF_SIZE, U8 MAX_DEPTH>
class AllocatorBuddy : public AllocatorBuddyBase<LEAF_SIZE, MAX_DEPTH, AllocatorBuddy<LEAF_SIZE, MAX_DEPTH>>
{
public:
    using ThisT = AllocatorBuddy<LEAF_SIZE, MAX_DEPTH>;
    using Base = AllocatorBuddyBase<LEAF_SIZE, MAX_DEPTH, AllocatorBuddy<LEAF_SIZE, MAX_DEPTH>>;

    AllocatorBuddy()
        : Base{}
        , m_Memory{ nullptr }
        , m_MemorySize{ 0 }
        , m_MetaData{ nullptr }
        , m_ChunksStart{ nullptr }
    {}

    AllocatorBuddy(void* memory, U64 memorySize)
        : Base{}
        , m_Memory{ memory }
        , m_MemorySize{ memorySize }
        , m_MetaData{ nullptr }
        , m_ChunksStart{ nullptr }
    {
        DRE_ASSERT(memorySize >= AllocatorBuddy::RequiredMemorySize(), "Not enough memory provided to AllocatorBuddy");

        MemZero(m_Memory, sizeof(m_MemorySize));
        m_MetaData = reinterpret_cast<typename Base::MetaData*>(PtrAlign(memory, alignof(typename Base::MetaData)));
        m_ChunksStart = ChunksStart();

        Base::Reset();
    }

    Base::MetaData& GetMetaDataImpl()
    {
        return *m_MetaData;
    }

    Base::MetaChunkHeader* GetChunkHeaderImpl(U64 chunkOffset)
    {
        return reinterpret_cast<typename Base::MetaChunkHeader*>(PtrAdd(m_ChunksStart, chunkOffset));
    }

    inline static constexpr U64 RequiredMemorySize()
    {
        return Base::RootChunkSize() + Base::RootChunkAlignment() + sizeof(typename Base::MetaData) + alignof(typename Base::MetaData);
    }

    inline void* ChunksStart()
    {
        return PtrAlign(PtrAdd(m_Memory, sizeof(typename Base::MetaData) + alignof(typename Base::MetaData)), Base::RootChunkAlignment());
    }

    inline U64 GetOffsetFromPtr(void* chunk)
    {
        return PtrDifference(chunk, m_ChunksStart);
    }

    void* Alloc(U64 size, U32 alignment)
    {
        U64 const offset = Base::Alloc(size, alignment);
        return offset != Base::INVALID_OFFSET ? PtrAdd(m_ChunksStart, offset) : nullptr;
    }

    void Free(void* chunk)
    {
        Base::Free(GetOffsetFromPtr(chunk));
    }

    template<typename T, typename... TArgs>
    inline T* Alloc(TArgs&&... args)
    {
        void* memory = ThisT::Alloc(sizeof(T), alignof(T));
        new (memory) T{ std::forward<TArgs>(args)... };
        return reinterpret_cast<T*>(memory);
    };

    template<typename T>
    inline void FreeObject(T* obj)
    {
        obj->~T();
        ThisT::Free(obj);
    }


    void*                       m_Memory;
    U64                         m_MemorySize;
    typename Base::MetaData*    m_MetaData;
    void*                       m_ChunksStart;
};

DRE_END_NAMESPACE

