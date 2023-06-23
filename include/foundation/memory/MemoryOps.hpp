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

inline std::uint32_t BitReverse(std::uint32_t x)
{
    x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
    x = (((x & 0xcccccccc) >> 2) | ((x & 0x33333333) << 2));
    x = (((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4));
    x = (((x & 0xff00ff00) >> 8) | ((x & 0x00ff00ff) << 8));

    return((x >> 16) | (x << 16));
}

inline std::uint8_t BitReverse(std::uint8_t x)
{
    x = (x & 0xF0) >> 4 | (x & 0x0F) << 4;
    x = (x & 0xCC) >> 2 | (x & 0x33) << 2;
    x = (x & 0xAA) >> 1 | (x & 0x55) << 1;
    return x;
}

DRE_END_NAMESPACE

