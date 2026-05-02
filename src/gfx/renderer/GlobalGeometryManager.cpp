#include <gfx\renderer\GlobalGeometryManager.hpp>

#include <foundation\memory\MemoryOps.hpp>
#include <gfx\GraphicsManager.hpp>
#include <engine\data\Geometry.hpp>

namespace GFX
{

GlobalGeometry::GeometryGPU::GeometryGPU(GlobalGeometry* manager, VKW::BufferResource* buffer,
    DRE::U64 vertexOffset, DRE::U32 vertexCount,
    DRE::U64 indexOffset, DRE::U32 indexCount)
    : m_GlobalGeometryManager{ manager }
    , m_ParentBuffer{ buffer }
    , m_VertexOffset{ vertexOffset }
    , m_VertexCount{ vertexCount }
    , m_IndexOffset{ indexOffset }
    , m_IndexCount{ indexCount }
{

}

GlobalGeometry::GeometryGPU::GeometryGPU()
    : m_GlobalGeometryManager{ nullptr }
    , m_ParentBuffer{ nullptr }
    , m_VertexOffset{ 0 }
    , m_VertexCount{ 0 }
    , m_IndexOffset{ 0 }
    , m_IndexCount{ 0 }
{

}

GlobalGeometry::GlobalGeometry(VKW::Context* loadingContext, VKW::Device* device, UploadArena* uploadArena)
    : DeviceChild{ device }
    , m_UploadArena{ uploadArena }
    , m_LoadingContext{ loadingContext }
    , m_GeometryMap{ &DRE::g_MainAllocator }
    , m_MainGeometryBuffer{ nullptr }
    , m_PendingUpdates{ &DRE::g_FrameScratchAllocator }
{
    VKW::ResourcesController* controller = m_ParentDevice->GetResourcesController();
    m_MainGeometryBuffer = controller->CreateBuffer(decltype(m_MainGeometryAllocator)::RootChunkSize(), VKW::BufferUsage::VERTEX_INDEX, "main_geometry");
}

GlobalGeometry::~GlobalGeometry()
{
    VKW::ResourcesController* controller = m_ParentDevice->GetResourcesController();
    controller->FreeBuffer(m_MainGeometryBuffer);
}

void GlobalGeometry::FreeGeometry(GeometryGPU& geometry)
{
    m_MainGeometryAllocator.Free(geometry.m_VertexOffset);
    m_MainGeometryAllocator.Free(geometry.m_IndexOffset);
}

std::uint64_t GlobalGeometry::GetMainBufferAddress() const
{
    return m_MainGeometryBuffer->gpuAddress_;
}

GlobalGeometry::GeometryGPU* GlobalGeometry::FindOrUploadGeometry(Data::Geometry* source)
{
    auto result = m_GeometryMap.Find(source);
    if (result.value == nullptr)
    {
        return ScheduleGeometryUpload(source);
    }
    else
    {
        return result.value;
    }
}

GlobalGeometry::GeometryGPU* GlobalGeometry::ScheduleGeometryUpload(Data::Geometry* source)
{
    DRE::U32 const requiredMemorySize = source->GetVertexSizeInBytes() + source->GetIndexSizeInBytes() + 8;
    auto geometryMemory = g_GraphicsManager->GetUploadArena().AllocateTransientRegion(g_GraphicsManager->GetCurrentFrameID(), requiredMemorySize, 256);

    void* vertexStart = DRE::PtrAlign(geometryMemory.m_MappedRange, 4);
    DRE::MemCpy(vertexStart, source->GetVertexData(), source->GetVertexSizeInBytes());

    void* indexStart = DRE::PtrAlign(DRE::PtrAdd(vertexStart, source->GetVertexSizeInBytes()), 4);
    DRE::U32 indexStartOffset = 0; // offset to copy indicies from
    if (source->GetIndexSizeInBytes() > 0)
    {
        DRE::MemCpy(indexStart, source->GetIndexData(), source->GetIndexSizeInBytes());
        indexStartOffset = DRE::U32(DRE::PtrDifference(indexStart, vertexStart));
    }

    DRE::U64 vertexOffset = m_MainGeometryAllocator.Alloc(source->GetVertexSizeInBytes(), 256);
    DRE_ASSERT(vertexOffset != m_MainGeometryAllocator.INVALID_OFFSET, "GlobalGeometry allocator is out of space");

    DRE::U64 indexOffset = m_MainGeometryAllocator.INVALID_OFFSET;
    if (source->GetIndexSizeInBytes() > 0)
    {
        indexOffset = m_MainGeometryAllocator.Alloc(source->GetIndexSizeInBytes(), 256);
        DRE_ASSERT(indexOffset != m_MainGeometryAllocator.INVALID_OFFSET, "GlobalGeometry allocator is out of space");
    }



    GeometryGPU geometryGPU{
        this, m_MainGeometryBuffer,
        vertexOffset, source->GetVertexCount(),
        indexOffset, source->GetIndexCount() };

    GeometryGPU& result = m_GeometryMap.Emplace(source, geometryGPU);

    m_PendingUpdates.EmplaceBack(geometryMemory, &result, indexStartOffset);
    return &result;
}

void GlobalGeometry::UpdateGPUGeometry(VKW::Context& context)
{
    if (!m_PendingUpdates.Empty())
    {
        context.CmdMemoryDependency(
            VKW::RESOURCE_ACCESS_GENERIC_WRITE, VKW::STAGE_ALL_GLOBAL,
            VKW::RESOURCE_ACCESS_GENERIC_RW, VKW::STAGE_ALL_GLOBAL);
    }

    for (std::uint32_t i = 0, count = m_PendingUpdates.Size(); i < count; i++)
    {
        UpdateEntry& entry = m_PendingUpdates[i];
        entry.m_TransientUpload.FlushCaches();

        VKW::BufferResource* const dstBuffer = m_MainGeometryBuffer;
        VKW::BufferResource* const srcBuffer = entry.m_TransientUpload.m_Buffer;

        DRE::U32 const dstVertexOffset = entry.m_GeometryGPU->GetVertexOffset();
        DRE::U32 const dstVertexSize = entry.m_GeometryGPU->GetVertexCount() * sizeof(Data::DREVertex);
        DRE::U32 const srcVertexOffset = entry.m_TransientUpload.m_OffsetInBuffer;
        context.CmdCopyBufferToBuffer(dstBuffer, dstVertexOffset, srcBuffer, srcVertexOffset, dstVertexSize);

        if (entry.m_IndexStart != 0)
        {
            DRE::U32 const dstIndexOffset = entry.m_GeometryGPU->GetIndexOffset();
            DRE::U32 const dstIndexSize = entry.m_GeometryGPU->GetIndexCount() * sizeof(DRE::U32);
            DRE::U32 const srcIndexOffset = entry.m_TransientUpload.m_OffsetInBuffer + entry.m_IndexStart;
            context.CmdCopyBufferToBuffer(dstBuffer, dstIndexOffset, srcBuffer, srcIndexOffset, dstIndexSize);
        }
    }

    if (!m_PendingUpdates.Empty())
    {
        context.CmdMemoryDependency(
            VKW::RESOURCE_ACCESS_GENERIC_WRITE, VKW::STAGE_TRANSFER,
            VKW::RESOURCE_ACCESS_GENERIC_READ, VKW::STAGE_ALL_GLOBAL);
    }

    m_PendingUpdates.Clear();
}

} // namespace GFX

