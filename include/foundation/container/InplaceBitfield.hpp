#pragma once

#include <foundation\Common.hpp>
#include <foundation\memory\MemoryOps.hpp>

DRE_BEGIN_NAMESPACE


/*
*

*
*/
template<U32 BIT_COUNT>
class InplaceBitfield
{
    static_assert(BIT_COUNT > 0, "InplaceBitfield storage size can't be ZERO.");

public:
    inline InplaceBitfield()
        : m_Storage{ 0 }
    {
    }

    inline ~InplaceBitfield()
    {
    }

    InplaceBitfield(InplaceBitfield<BIT_COUNT> const& rhs)
        : m_Storage{ 0 }
    {
        operator=(rhs);
    }

    InplaceBitfield(InplaceBitfield<BIT_COUNT>&& rhs)
        : m_Storage{ 0 }
    {
        operator=(DRE_MOVE(rhs));
    }

    InplaceBitfield<BIT_COUNT>& operator=(InplaceBitfield<BIT_COUNT> const& rhs)
    {
        MemCpy(m_Storage, rhs.m_Storage, rhs.SizeInBytes());

        return *this;
    }

    InplaceBitfield<BIT_COUNT>& operator=(InplaceBitfield<BIT_COUNT>&& rhs)
    {
        MemCpy(m_Storage, rhs.m_Storage, rhs.SizeInBytes());

        return *this;
    }

    void SetTrue(U32 i) { m_Storage[i / 8] = m_Storage[i / 8] | (1u << (i % 8)); }
    void SetFalse(U32 i) { m_Storage[i / 8] = m_Storage[i / 8] & ~(1u << (i % 8)); }
    bool Get(U32 i) { return static_cast<bool>(m_Storage[i / 8] & (1u << (i % 8))); }

    void Clear()
    {
       MemZero(m_Storage, SizeInBytes());
    }

    static inline U32 constexpr SizeInBytes()
    {
        return StorageSize();
    }

    static inline U32 constexpr StorageSize()
    {
        return BIT_COUNT / 8 + 1;
    }

    static U32 constexpr Capacity()
    {
         return BIT_COUNT;
    }

private:
    U8  m_Storage[StorageSize()];
};

DRE_END_NAMESPACE

