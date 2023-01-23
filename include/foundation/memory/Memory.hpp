#pragma once

#include <foundation\memory\AllocatorLinear.hpp>
#include <foundation\memory\AllocatorScopeStack.hpp>
#include <foundation\memory\AllocatorBuddy.hpp>


#define DRE_MALLOC(size) std::malloc(size)
#define DRE_FREE(memory) std::free(memory)


DRE_BEGIN_NAMESPACE


/*
*
* Root application memory control header file
* Defining allocation contexts
*
*/


void InitializeGlobalMemory();
void TerminateGlobalMemory();


// To be used with all persistent stuff. WARNING, REQUIRES MANUAL DESTRUCTION
extern AllocatorLinear                  g_PersistentDataAllocator;
extern AllocatorLinear                  g_FrameScratchAllocator;


U64 constexpr MAIN_ALLOCATOR_LEAF_SIZE  = 1024 * 64;
U64 constexpr MAIN_ALLOCATOR_MAX_DEPTH  = 12;
using  DefaultAllocator                 = AllocatorBuddy<MAIN_ALLOCATOR_LEAF_SIZE, MAIN_ALLOCATOR_MAX_DEPTH>;
extern DefaultAllocator                 g_MainAllocator;





// To be used for persistent data exchange between threads on JOIN stages, etc.
U64 constexpr DATA_EXCHANGE_LEAF_SIZE   = 1024;
U64 constexpr DATA_EXCHANGE_MAX_DEPTH   = 5;
using  DataExchangeAllocatorBuddy       = AllocatorBuddy<DATA_EXCHANGE_LEAF_SIZE, DATA_EXCHANGE_MAX_DEPTH>;
extern DataExchangeAllocatorBuddy       g_MultithreadDataExchangeAllocator;

DRE_END_NAMESPACE
