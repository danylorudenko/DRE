#pragma once

#include <cassert>
#include <cstdio>
#include <cstdlib>  // for rand, srand
#include <ctime>    // for time()

#include <foundation\Common.hpp>

#include <foundation\memory\AllocatorBuddy.hpp>
#include <foundation\memory\ElementAllocator.hpp>

bool AllocatorBuddyTest()
{
    bool testPassed = true;

    std::srand(static_cast<unsigned>(std::time(nullptr)));

    using AllocatorBuddySetup = DRE::AllocatorBuddy<1024, 7>;
    static char arena[AllocatorBuddySetup::RequiredMemorySize()];

    AllocatorBuddySetup allocator(arena, AllocatorBuddySetup::RequiredMemorySize());
    allocator.Reset();

    // --- Long-Term Random Allocation/Free Test ---
    // We simulate a long-lived allocator by randomly allocating and freeing blocks.
    // Use an array to track live allocations.
    const int maxAllocations = 1024;
    void* allocations[maxAllocations] = { 0 };
    int allocCount = 0;
    const int iterations = 1000000;  // Long test loop

    // Allowed allocation sizes (all powers of two, never less than 256 bytes).
    auto randomSize = []() {
        int exp = 8 + (std::rand() % (AllocatorBuddySetup::MaxDepth() + 1));
        return std::min(static_cast<size_t>(1) << exp, AllocatorBuddySetup::RootChunkSize());
    };

    for (int iter = 0; iter < iterations; ++iter)
    {
        int op = (std::rand() % 3) == 2 ? 1 : 0;  // 0: allocate, 1: free

        if (op == 0 && allocCount < maxAllocations)
        {
            // Request an allocation of a random power-of-two size (>=256 bytes).
            size_t size = randomSize();
            if (size < 256) size = 256; // enforce minimum

            void* ptr = allocator.Alloc(size, 256);
            printf("allocated %i, size %i\n", allocCount, (int)size);
            if (ptr != nullptr)
            {
                // Check that this allocation does not duplicate any existing allocation.
                for (int j = 0; j < allocCount; ++j)
                {
                    assert(allocations[j] != ptr && "Allocator returned a block that's already allocated!");
                    if (allocations[j] == ptr)
                        testPassed = false;

                }
                allocations[allocCount++] = ptr;
            }
        }
        else if (op == 1 && allocCount > 0)
        {
            // Free a random allocation from our array.
            int index = std::rand() % allocCount;
            void* ptr = allocations[index];
            printf("freed %i\n", index);
            allocator.Free(ptr);
            
            allocations[index] = allocations[--allocCount];
        }
    }

    // Free any remaining allocations.
    for (int i = 0; i < allocCount; ++i)
    {
        allocator.Free(allocations[i]);
    }
    allocCount = 0;

    // As a final check, perform one more allocation.
    void* finalPtr = allocator.Alloc(256, 256);
    assert(finalPtr != nullptr && "Final allocation failed after long-term random operations.");
    allocator.Free(finalPtr);

    if (testPassed)
        printf("Long-term random allocation/free test passed with unique allocations verified.\n");

    return testPassed;
}

bool ElementAllocatorBuddyTest()
{
    bool testPassed = true;

    std::srand(static_cast<unsigned>(std::time(nullptr)));

    static constexpr DRE::U8 MAX_DEPTH = 7;

    using BuddyElementSetup = DRE::BuddyAllocatorOffsets<1024, MAX_DEPTH>;

    BuddyElementSetup allocator;

    // --- Long-Term Random Allocation/Free Test ---
    // We simulate a long-lived allocator by randomly allocating and freeing blocks.
    // Use an array to track live allocations.
    const int maxAllocations = 1024;
    DRE::U64 allocations[maxAllocations] = { 0 };
    int allocCount = 0;
    const int iterations = 1000000;  // Long test loop

    // Allowed allocation sizes (all powers of two, never less than 256 bytes).
    auto randomSize = []() {
        int exp = 8 + (std::rand() % (MAX_DEPTH + 1));
        return std::min(static_cast<size_t>(1) << exp, BuddyElementSetup::RootChunkSize());
    };

    for (int iter = 0; iter < iterations; ++iter)
    {
        int op = (std::rand() % 3) == 2 ? 1 : 0;  // 0: allocate, 1: free

        if (op == 0 && allocCount < maxAllocations)
        {
            // Request an allocation of a random power-of-two size (>=256 bytes).
            size_t size = randomSize();
            if (size < 256) size = 256; // enforce minimum

            DRE::U64 element = allocator.Alloc(size, 256);
            if (element != BuddyElementSetup::INVALID_OFFSET)
            {
                printf("allocated %i, no.%i, size %i\n", (int)element, allocCount, (int)size);
                allocator.PrintIsFreeState();
                // Check that this allocation does not duplicate any existing allocation.
                for (int j = 0; j < allocCount; ++j)
                {
                    assert(allocations[j] != element && "Allocator returned a block that's already allocated!");
                    if (allocations[j] == element)
                        testPassed = false;

                }
                allocations[allocCount++] = element;
            }
        }
        else if (op == 1 && allocCount > 0)
        {
            // Free a random allocation from our array.
            int index = std::rand() % allocCount;
            DRE::U64 element = allocations[index];
            allocator.Free(element);
            printf("freed %i\n", index);
            allocator.PrintIsFreeState();
            allocations[index] = allocations[--allocCount];
        }
    }

    // Free any remaining allocations.
    for (int i = 0; i < allocCount; ++i)
    {
        allocator.Free(allocations[i]);
    }
    allocCount = 0;

    // As a final check, perform one more allocation.
    DRE::U64 finalElement = allocator.Alloc(256, 256);
    assert(finalElement != BuddyElementSetup::INVALID_OFFSET && "Final allocation failed after long-term random operations.");
    allocator.Free(finalElement);

    if (testPassed)
        printf("Long-term random allocation/free test passed with unique allocations verified.\n");

    return testPassed;
}
