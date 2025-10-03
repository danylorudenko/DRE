#pragma once

#include <foundation\Common.hpp>
#include <foundation\math\SimpleMath.hpp>
#include <foundation\memory\MemoryOps.hpp>
#include <foundation\memory\AllocatorBuddy.hpp>

DRE_BEGIN_NAMESPACE


////////////////////////////////////////////////
// FreeListOffsetAllocator
////////////////////////////////////////////////
template<DRE::U32 LIST_SIZE>
class FreeListOffsetAllocator
{
    static_assert(LIST_SIZE - 1 < DRE_U32_MAX);

    static constexpr DRE::U32 INVALID_ELEMENT = DRE_U32_MAX;

public:
    FreeListOffsetAllocator()
    {
        Reset();
    }

    FreeListOffsetAllocator(FreeListOffsetAllocator&& rhs)
    {
        operator=(std::move(rhs));
    }

    FreeListOffsetAllocator& operator=(FreeListOffsetAllocator&& rhs)
    {
        std::memcpy(this, &rhs, sizeof(*this));
        return *this;
    }

    DRE::U32 Allocate()
    {
        DRE_ASSERT(firstFree_ != INVALID_ELEMENT, "FreeListOffsetAllocator has no free entries.");

        DRE::U32 result = firstFree_;
        firstFree_ = storage_[firstFree_];
        return result;
    }

    void Free(DRE::U32 element)
    {
        storage_[element] = firstFree_;
        firstFree_ = element;
    }

    void Reset()
    {
        for (DRE::U32 i = 0; i < LIST_SIZE - 1; i++)
        {
            storage_[i] = i + 1;
        }
        storage_[LIST_SIZE - 1] = INVALID_ELEMENT;

        firstFree_ = 0;
    }

private:
    DRE::U32 storage_[LIST_SIZE];
    DRE::U32 firstFree_;
};


////////////////////////////////////////////////
// LinearOffsetAllocator
////////////////////////////////////////////////
template<DRE::U32 SIZE>
class LinearOffsetAllocator
{
public:
    DRE::U32 Allocate(DRE::U32 count)
    {
        DRE_ASSERT((nextFree_ + count) < SIZE, "LinearOffsetAllocator has no free space left.");

        DRE::U32 result = nextFree_;
        nextFree_ += count;
        return result;
    }

    void Free(DRE::U32 element) {}

    void Reset()
    {
        nextFree_ = 0;
    }

private:
    DRE::U32 nextFree_;
};

template<DRE::U32 SIZE>
class DummyElementsAllocator
{
    static constexpr DRE::U32 INVALID_ELEMENT = DRE_U32_MAX;
public:
    DRE::U32    Allocate  (DRE::U32 count) { /* noop */ return INVALID_ELEMENT; }
    void        Free      (DRE::U32 element) { /* noop */ }
    void        Reset     () { /* noop */ }
};


////////////////////////////////////////////////
// BuddyOffsetAllocator
////////////////////////////////////////////////
template<U64 LEAF_SIZE, U8 MAX_DEPTH>
class BuddyOffsetAllocator : public AllocatorBuddyBase<LEAF_SIZE, MAX_DEPTH, BuddyOffsetAllocator<LEAF_SIZE, MAX_DEPTH>>
{
public:
    using Base = AllocatorBuddyBase<LEAF_SIZE, MAX_DEPTH, BuddyOffsetAllocator<LEAF_SIZE, MAX_DEPTH>>;

    BuddyOffsetAllocator()
        : Base{}
    {
        MemZero(&m_MetaData, sizeof(m_MetaData));
        MemZero(m_MetaHeaderStorage, sizeof(m_MetaHeaderStorage));

        Base::Reset();
    }

    Base::MetaData& GetMetaDataImpl()
    {
        return m_MetaData;
    }

    BuddyOffsetAllocator& operator=(BuddyOffsetAllocator&& rhs)
    {
        DRE_ASSERT(false, "Don't copy allocators.");
        return *this;
    }

    Base::MetaChunkHeader* GetChunkHeaderImpl(U64 chunkOffset)
    {
        return m_MetaHeaderStorage + (chunkOffset / Base::LeafSize());
    }

    Base::MetaData m_MetaData;
    Base::MetaChunkHeader m_MetaHeaderStorage[Base::LeavesCount()];

};


DRE_END_NAMESPACE
