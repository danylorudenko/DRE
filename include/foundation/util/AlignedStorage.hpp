#pragma once

#include <foundation\Common.hpp>

DRE_BEGIN_NAMESPACE

template<typename T>
struct AlignedStorage
{
    static constexpr U32 TSize  = sizeof(T);
    static constexpr U32 TAlign = alignof(T);

    alignas(T)
    U8  m_Storage[TSize];

    inline operator T&() { return *Ptr(); }
    inline operator T const&() const { return *Ptr(); }

    inline T* Ptr() { return reinterpret_cast<T*>(m_Storage); }
    inline T const* Ptr() const { return reinterpret_cast<T*>(m_Storage); }

    inline void Destroy() { reinterpret_cast<T*>(m_Storage)->~T(); }
};

DRE_END_NAMESPACE

