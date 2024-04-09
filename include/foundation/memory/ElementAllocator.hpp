#pragma once

#include <foundation\Common.hpp>

DRE_BEGIN_NAMESPACE

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

DRE_END_NAMESPACE
