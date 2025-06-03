#pragma once

#include <foundation\Common.hpp>
#include <foundation\math\SimpleMath.hpp>
#include <foundation\memory\MemoryOps.hpp>
#include <foundation\memory\AllocatorBuddy.hpp>

DRE_BEGIN_NAMESPACE


////////////////////////////////////////////////
// FreeListOffsetAllocator
////////////////////////////////////////////////
template<std::uint16_t LIST_SIZE>
class FreeListOffsetAllocator
{
    static_assert(LIST_SIZE - 1 < DRE_U16_MAX);

    static constexpr std::uint16_t INVALID_ELEMENT = DRE_U16_MAX;

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

    std::uint16_t Allocate()
    {
        DRE_ASSERT(firstFree_ != INVALID_ELEMENT, "FreeListOffsetAllocator has no free entries.");

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
// LinearOffsetAllocator
////////////////////////////////////////////////
template<std::uint16_t SIZE>
class LinearOffsetAllocator
{
public:
    std::uint16_t Allocate(std::uint16_t count)
    {
        DRE_ASSERT((nextFree_ + count) < SIZE, "LinearOffsetAllocator has no free space left.");

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
