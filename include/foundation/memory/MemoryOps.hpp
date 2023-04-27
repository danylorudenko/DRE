#pragma once

#include <foundation\Common.hpp>
#include <foundation\memory\Pointer.hpp>

DRE_BEGIN_NAMESPACE

inline void DreMemZero(void* mem, U64 size)
{
    ::ZeroMemory(mem, size);
}

inline void DreMemSet(void* mem, U64 size, U32 val)
{
    ::FillMemory(mem, size, val);
}

inline void WriteMemorySequence(void*& memory, void const* data, std::uint32_t size)
{
    std::memcpy(memory, data, size);
    memory = DRE::PtrAdd(memory, size);
}

DRE_END_NAMESPACE

