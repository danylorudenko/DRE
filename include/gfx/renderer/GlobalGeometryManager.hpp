#pragma once

#include <foundation\Common.hpp>

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>

#include <foundation\memory\AllocatorLinear.hpp>
#include <foundation\Container\Vector.hpp>
#include <foundation\memory\OffsetAllocator.hpp>

#include <vk_wrapper\resources\Resource.hpp>
#include <gfx\DeviceChild.hpp>

namespace VKW
{
    class Context;
    class DescriptorManager;
}

namespace GFX
{

class PersistentStorage;

/////////////////////////////
class GlobalGeometry
    : public NonMovable
    , public DeviceChild
{
public:
    static constexpr DRE::U64 PERSISTENT_GEOMETRY_SIZE = 16384 * (1 << 13); // == 134,217,728  ~134MB

    class GeometryGPU
    {
        friend class GlobalGeometry;

    private:
        GeometryGPU(GlobalGeometry* manager, std::uint16_t id);

        GlobalGeometry*         m_GlobalGeometryManager;
        VKW::BufferResource*    m_ParentBuffer;
        std::uint32_t           m_Offset;
        std::uint32_t           m_Size;
    };

public:
    GlobalGeometry();

    GeometryGPU AllocateGeometry(std::uint32_t size);
    GeometryGPU AllocatePersistentGeometry(std::uint32_t size);
    void FreeGeometry(GeometryGPU& geometry);

    DRE::U32 GetGeometryCount() const;
    DRE::U64 GetBufferAddress() const;

private:
    VKW::BufferResource* m_PersistentGeometryBuffer;
    DRE::LinearOffsetAllocator<PERSISTENT_GEOMETRY_SIZE> m_PersistentGeometryAllocator;

    VKW::BufferResource* m_MainGeometryBuffer;
    DRE::BuddyOffsetAllocator<16384, 16> m_MainGeometryAllocator;
};

}
