#pragma once

#include <foundation\Common.hpp>
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

template<typename PayloadT, DRE::U32 MAX_COUNT, DRE::U32 QUEUE_SIZE>
class GPUInstanceAllocator
    : public NonMovable
    , public NonCopyable
{
public:
    using Type = GPUInstanceAllocator<PayloadT, MAX_COUNT, QUEUE_SIZE>;

    class Payload
    {
    public:
        using ManagerType = Type;

        Payload(ManagerType* manager, DRE::U64 addressGPU, DRE::U32 id)
            : m_Manager{ manager }
            , m_AddressGPU{ addressGPU }
            , m_id{ id }
        { }

        void ScheduleUpdate(PayloadT const& data)
        {
            m_Manager->ScheduleUpdate(m_id, data);
        }

        DRE::U32     GetID() const { return m_id; }
        DRE::U64     GetAddressGPU() const { return m_AddressGPU; }
        ManagerType* GetManager() { return m_Manager; }

    protected:
        Type*               m_Manager;
        DRE::U64            m_AddressGPU;
        DRE::U32            m_id;
    };

    friend class Payload;

    GPUInstanceAllocator(PersistentStorage* storage)
        : m_PersistentAllocation{ storage->AllocateRegion(MAX_COUNT * sizeof(PayloadT)) }
        , m_Count{ 0 }
    { }

    DRE::U32 AllocateID()
    {
        ++m_Count;
        return m_ElementAllocator.Allocate();
    }

    void FreeID(DRE::U32 id)
    {
        --m_Count;
        m_ElementAllocator.Free(id);
    }

    DRE::U64 GetBufferAddress() const { return m_PersistentAllocation.GetGPUAddress(); }
    DRE::U32 GetCount() const { return m_Count; }

    void ScheduleUpdate(DRE::U32 id, PayloadT const& payload)
    {
        m_UpdateQueue.EmplaceBack(id, payload);
    }

    void FlushUpdates(VKW::Context& context)
    {
        for (DRE::U32 i = 0, count = m_UpdateQueue.Size(); i < count; ++i)
        {
            UpdateEntry& entry = m_UpdateQueue[i];
            m_PersistentAllocation.Update(context, entry.id * sizeof(PayloadT), &entry.payload, sizeof(PayloadT));
        }
        m_UpdateQueue.Clear();
    }

protected:
    struct UpdateEntry
    {
        DRE::U32 id;
        PayloadT payload;
    };

    PersistentStorage::Allocation m_PersistentAllocation;
    DRE::FreeListOffsetAllocator<MAX_COUNT> m_ElementAllocator;
    DRE::U32 m_Count;
    DRE::InplaceVector<UpdateEntry, QUEUE_SIZE> m_UpdateQueue;
};

} // namespace GFX

