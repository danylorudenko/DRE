#include <foundation\memory\Memory.hpp>

DRE_BEGIN_NAMESPACE


U64 constexpr FRAME_SCRATCH_ARENA_SIZE  = 1024 * 1024 * 64;
U64 constexpr PERSISTENT_ARENA_SIZE     = 1024 * 1024 * 64;
U64 constexpr THREAD_LOCAL_ARENA_SIZE   = 1024 * 1024 * 16;




thread_local void*               g_ThreadLocalArena = nullptr;
thread_local AllocatorScopeStack g_ThreadLocalStackAllocator;


void* g_PersistentArena     = nullptr;
void* g_FrameScratchArena   = nullptr;
void* g_MainArena           = nullptr;
void* g_DataExchangeArena   = nullptr;

AllocatorLinear                 g_PersistentDataAllocator;
AllocatorLinear                 g_FrameScratchAllocator;
#ifndef DRE_DEBUG_MAIN_ALLOCATOR
DefaultAllocator                g_MainAllocator;
#else
AllocatorSystem                 g_MainAllocator;
#endif



void InitializeGlobalMemory()
{
    g_PersistentArena = DRE_MALLOC(PERSISTENT_ARENA_SIZE);
    g_PersistentDataAllocator = AllocatorLinear(g_PersistentArena, PERSISTENT_ARENA_SIZE);

    g_FrameScratchArena = DRE_MALLOC(FRAME_SCRATCH_ARENA_SIZE);
    g_FrameScratchAllocator = AllocatorLinear(g_FrameScratchArena, FRAME_SCRATCH_ARENA_SIZE);

#ifndef DRE_DEBUG_MAIN_ALLOCATOR
    U64 constexpr mainAllocatorMemorySize = DefaultAllocator::RequiredMemorySize();
    g_MainArena = DRE_MALLOC(mainAllocatorMemorySize);
    g_MainAllocator = DefaultAllocator{ g_MainArena, mainAllocatorMemorySize };
#endif
}

void TerminateGlobalMemory()
{
    DRE_FREE(g_DataExchangeArena);
    DRE_FREE(g_MainArena);
    DRE_FREE(g_FrameScratchArena);
    DRE_FREE(g_PersistentArena);
}

DRE_END_NAMESPACE
