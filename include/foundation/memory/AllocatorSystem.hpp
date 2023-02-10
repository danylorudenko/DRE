#pragma once

#include <foundation\Common.hpp>

DRE_BEGIN_NAMESPACE

/*
*
* System allocation implementation.
* Mostly for debugging
* Just calls to malloc, free internally
*
* Basic interface:
*
*   + Alloc (size, alignment)
*   + Free  ()
*   + Reset ()
*   + MemorySize()
*
*/
class AllocatorSystem
{
public:
    inline void* Alloc(U64 size, U32 alignment)
    {
        DRE_ASSERT(IsValidAlignment(alignment), "AllocatorLinear: received invalid allocation alignment.");

        return _aligned_malloc(size, alignment);
    }

    inline void Reset()
    {
    }

    inline void Free(void* allocation)
    {
        _aligned_free(allocation);
    }

    inline static bool IsValidAlignment(U32 alignment)
    {
        return IsPowOf2(alignment);
    }
};

DRE_END_NAMESPACE

