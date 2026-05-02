#pragma once

#include <foundation\Common.hpp>

#include <foundation\class_features\NonMovable.hpp>

#include <foundation\memory\Memory.hpp>
#include <foundation\memory\AllocatorLinear.hpp>
#include <foundation\memory\OffsetAllocator.hpp>
#include <foundation\container\Vector.hpp>
#include <foundation\container\HashTable.hpp>

#include <vk_wrapper\resources\Resource.hpp>

#include <gfx\DeviceChild.hpp>
#include <gfx\buffer\TransientArena.hpp>

namespace VKW
{
    class Context;
}

namespace Data
{
class Geometry;
}

namespace GFX
{

/////////////////////////////
class GlobalGeometry
    : public NonMovable
    , public DeviceChild
{
public:
    class GeometryGPU
    {
    public:
        friend class GlobalGeometry;

        GeometryGPU();

        VKW::BufferResource*    GetBuffer() const { return m_ParentBuffer; }

        DRE::U64                GetVertexOffset() const { return m_VertexOffset; }
        DRE::U32                GetVertexCount() const { return m_VertexCount; }
        DRE::U64                GetVertexGPUAddress() const { return m_ParentBuffer->gpuAddress_ + m_VertexOffset; }

        DRE::U64                GetIndexOffset() const { return m_IndexOffset; }
        DRE::U32                GetIndexCount() const { return m_IndexCount; }
        DRE::U64                GetIndexGPUAddress() const { return m_ParentBuffer->gpuAddress_ + m_IndexOffset; }

        // MAKE IT PRIVATE
    public:
        GeometryGPU(GlobalGeometry* manager, VKW::BufferResource* buffer,
            DRE::U64 vertexOffset, DRE::U32 vertexCount,
            DRE::U64 indexOffset, DRE::U32 indexCount);

        GlobalGeometry*         m_GlobalGeometryManager;
        VKW::BufferResource*    m_ParentBuffer;

        DRE::U64                m_VertexOffset;
        DRE::U32                m_VertexCount;

        DRE::U64                m_IndexOffset;
        DRE::U32                m_IndexCount;
    };

public:
    GlobalGeometry(VKW::Context* loadingContext, VKW::Device* device, UploadArena* uploadArena);

    void FreeGeometry(GeometryGPU& geometry);

    DRE::U64 GetMainBufferAddress() const;
    GeometryGPU* ScheduleGeometryUpload(Data::Geometry* source);
    GeometryGPU* FindOrUploadGeometry(Data::Geometry* source);

    void UpdateGPUGeometry(VKW::Context& context);

    virtual ~GlobalGeometry();

private:
    UploadArena* m_UploadArena;
    VKW::Context* m_LoadingContext;

    VKW::BufferResource* m_MainGeometryBuffer;
    DRE::BuddyOffsetAllocator<65536, 13> m_MainGeometryAllocator; // manages ~536MB  (536,870,912) Size of the allocator drives the size of GPU buffer

    DRE::HashTable<Data::Geometry*, GeometryGPU, DRE::DefaultAllocator> m_GeometryMap;

    struct UpdateEntry
    {
        UploadArena::Allocation m_TransientUpload;
        GeometryGPU* m_GeometryGPU;
        DRE::U32 m_IndexStart;
    };
    DRE::Vector<UpdateEntry, DRE::AllocatorLinear> m_PendingUpdates;
};

}
