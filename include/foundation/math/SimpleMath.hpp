#pragma once

#include <foundation\Common.hpp>

DRE_BEGIN_NAMESPACE

inline constexpr U32 NextPowOf2(U32 arg)
{
    arg--;
    arg |= arg >> 1;
    arg |= arg >> 2;
    arg |= arg >> 4;
    arg |= arg >> 8;
    arg |= arg >> 16;
    arg++;

    return arg;
}

template<typename T>
inline constexpr bool IsPowOf2(T arg)
{
    return arg != 0 && !(arg & (arg - 1));
}

inline constexpr U64 Log2(U64 value)
{
    return (value < 2) ? (1) : (1 + Log2(value / 2));
}

template<typename T>
inline constexpr T Align(T value, U32 alignment)
{
    return (T)(value + alignment - 1) & (~((T)alignment - 1));
}

template<typename T>
inline constexpr T Max(T lhs, T rhs)
{
    return lhs > rhs ? lhs : rhs;
}

template<typename T>
inline constexpr T Min(T lhs, T rhs)
{
    return lhs < rhs ? lhs : rhs;
}

DRE_END_NAMESPACE

