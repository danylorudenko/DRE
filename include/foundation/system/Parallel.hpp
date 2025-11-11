#pragma once

#include <foundation/Common.hpp>
#include <foundation/container/InplaceVector.hpp>
#include <foundation/math/SimpleMath.hpp>

#include <functional>
#include <future>
#include <type_traits>
#include <utility>

DRE_BEGIN_NAMESPACE

// Helper object that aggregates asynchronous tasks launched by ParallelFor and
// provides a unified wait primitive for them.
template<U32 MAX_PARALLEL_FACTOR>
class ParallelTaskGroup
{
public:
    using FutureType = std::future<void>;

    ParallelTaskGroup() = default;
    ParallelTaskGroup(ParallelTaskGroup const&) = delete;
    ParallelTaskGroup& operator=(ParallelTaskGroup const&) = delete;
    ParallelTaskGroup(ParallelTaskGroup&&) = default;
    ParallelTaskGroup& operator=(ParallelTaskGroup&&) = default;

    inline void Add(FutureType&& future)
    {
        m_Futures.EmplaceBack(DRE_MOVE(future));
    }

    inline void Wait()
    {
        U32 const count = m_Futures.Size();
        for (U32 i = 0; i < count; ++i)
        {
            m_Futures[i].get();
        }
    }

    inline bool Empty() const
    {
        return m_Futures.Empty();
    }

    inline U32 Size() const
    {
        return m_Futures.Size();
    }

private:
    InplaceVector<FutureType, MAX_PARALLEL_FACTOR> m_Futures;
};

// Executes the provided callable for each index in the range [0, count) either in
// parallel or sequentially depending on the provided flag.
// The callable must be invocable with a single parameter of type U32.
//
// Returns a task group that can be used to wait for all asynchronous operations to
// complete. In the sequential case the returned group will be empty and Wait() will
// return immediately.
template<U32 MAX_PARALLEL_FACTOR = 8, typename TFunc>
ParallelTaskGroup<MAX_PARALLEL_FACTOR> ParallelFor(U32 count, TFunc&& func, bool parallel, U32 chunkSize = 0)
{
    ParallelTaskGroup<MAX_PARALLEL_FACTOR> taskGroup{};

    if (count == 0)
    {
        return taskGroup;
    }

    using CallableT = std::decay_t<TFunc>;
    CallableT callable{ std::forward<TFunc>(func) };

    if (!parallel || count == 1)
    {
        for (U32 index = 0; index < count; ++index)
        {
            std::invoke(callable, index);
        }

        return taskGroup;
    }

    U32 const parallelFactor = DRE::Max<U32>(1, DRE::Min<U32>(MAX_PARALLEL_FACTOR, count));
    U32 const effectiveChunkSize = chunkSize != 0 ? chunkSize : ((count + parallelFactor - 1) / parallelFactor);

    for (U32 chunkID = 0; chunkID < parallelFactor; ++chunkID)
    {
        taskGroup.Add(std::async(std::launch::async, [chunkID, effectiveChunkSize, count, callable]() mutable
        {
            U32 const chunkStart = chunkID * effectiveChunkSize;
            if (chunkStart >= count)
            {
                return;
            }

            U32 const chunkEnd = DRE::Min<U32>(chunkStart + effectiveChunkSize, count);
            for (U32 index = chunkStart; index < chunkEnd; ++index)
            {
                std::invoke(callable, index);
            }
        }));
    }

    return taskGroup;
}

DRE_END_NAMESPACE

