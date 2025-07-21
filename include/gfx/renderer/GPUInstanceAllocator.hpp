#pragma once

#include <cstdint>

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>
#include <foundation\container\InplaceVector.hpp>
#include <foundation\memory\OffsetAllocator.hpp>

#include <gfx\buffer\PersistentStorage.hpp>

namespace VKW
{
class Context;
}

namespace GFX
{

template<typename PayloadT, std::uint32_t MAX_COUNT, std::uint32_t QUEUE_SIZE>
class GPUInstanceAllocator
    : public NonMovable
    , public NonCopyable
{
public:
    using PayloadType = PayloadT;

    GPUInstanceAllocator(PersistentStorage* storage)
        : m_PersistentAllocation{ storage->AllocateRegion(MAX_COUNT * sizeof(PayloadT)) }
        , m_Count{ 0 }
    { }

    std::uint16_t AllocateID()
    {
        ++m_Count;
        return m_ElementAllocator.Allocate();
    }

    void FreeID(std::uint16_t id)
    {
        --m_Count;
        m_ElementAllocator.Free(id);
    }

    std::uint64_t GetBufferAddress() const { return m_PersistentAllocation.GetGPUAddress(); }
    std::uint32_t GetCount() const { return m_Count; }

protected:
    void ScheduleUpdate(std::uint32_t id, PayloadT const& payload)
    {
        m_UpdateQueue.EmplaceBack(id, payload);
    }

    void FlushUpdates(VKW::Context& context)
    {
        for (std::uint32_t i = 0, count = m_UpdateQueue.Size(); i < count; ++i)
        {
            UpdateEntry& entry = m_UpdateQueue[i];
            m_PersistentAllocation.Update(context, entry.id * sizeof(PayloadT), &entry.payload, sizeof(PayloadT));
        }
        m_UpdateQueue.Clear();
    }

protected:
    struct UpdateEntry
    {
        std::uint32_t id;
        PayloadT payload;
    };

    PersistentStorage::Allocation m_PersistentAllocation;
    DRE::FreeListOffsetAllocator<MAX_COUNT> m_ElementAllocator;
    std::uint32_t m_Count;
    DRE::InplaceVector<UpdateEntry, QUEUE_SIZE> m_UpdateQueue;
};

} // namespace GFX

