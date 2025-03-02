#include <gfx/renderer/GlobalGeometryManager.hpp>

#include <vk_wrapper/descriptor/DescriptorManager.hpp>
#include <gfx/GraphicsManager.hpp>

/*
#include <common/geometry/geometry.h>

namespace GFX
{

GlobalGeometry::GlobalGeometry(PersistentStorage* storage)
    : m_PersistentAllocation{ storage->AllocateRegion(MAX_GEOMETRY * sizeof(S_GEOMETRY)) }
    , m_GeometryCount{ 0 }
{
}

GlobalGeometry::GeometryGPU GlobalGeometry::AllocateGeometry()
{
    std::uint16_t const id = m_ElementAllocator.Allocate();
    ++m_GeometryCount;
    return GeometryGPU{ this, id };
}

void GlobalGeometry::FreeGeometry(GeometryGPU& geometry)
{
    --m_GeometryCount;
    m_ElementAllocator.Free(geometry.m_id);
}

std::uint64_t GlobalGeometry::GetBufferAddress() const
{
    return m_PersistentAllocation.GetGPUAddress();
}

std::uint32_t GlobalGeometry::GetGeometryCount() const
{
    return m_GeometryCount;
}

void GlobalGeometry::ScheduleGeometryUpdate(std::uint16_t id)
{
    S_GEOMETRY geometryData;
    // TODO: Fill 'geometryData' with the appropriate geometry update information.
    
    m_GeometryUpdateQueue.EmplaceBack(id, geometryData);
}

void GlobalGeometry::UpdateGPUGeometry(VKW::Context& context)
{
    std::uint64_t baseAddress = m_PersistentAllocation.GetGPUAddress();

    for (std::uint32_t i = 0, count = m_GeometryUpdateQueue.Size(); i < count; i++)
    {
        GeometryUpdateEntry& entry = m_GeometryUpdateQueue[i];
        m_PersistentAllocation.Update(context, sizeof(S_GEOMETRY) * entry.id, &entry.payload, sizeof(S_GEOMETRY));
    }

    m_GeometryUpdateQueue.Clear();
}

///////////////////////////////////////////
///////////////////////////////////////////
///////////////////////////////////////////

GlobalGeometry::GeometryGPU::GeometryGPU(GlobalGeometry* manager, std::uint16_t id)
    : m_GlobalGeometryManager{ manager }
    , m_id{ id }
{
}

void GlobalGeometry::GeometryGPU::ScheduleUpdate()
{
    // Forward the update scheduling to the manager.
    // Example:
    // m_GlobalGeometryManager->ScheduleGeometryUpdate(m_id, );
}

} // namespace GFX
*/
