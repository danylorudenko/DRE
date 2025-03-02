#pragma once

#include <cstdint>

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>

#include <foundation\memory\AllocatorLinear.hpp>
#include <foundation\Container\Vector.hpp>
#include <foundation\memory\ElementAllocator.hpp>

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
    static constexpr std::uint32_t MAX_GEOMETRY = 1024;

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
    GlobalGeometry(PersistentStorage* storage);

    GeometryGPU AllocateGeometry(std::uint32_t size);
    GeometryGPU AllocatePersistentGeometry(std::uint32_t size);
    void FreeGeometry(GeometryGPU& geometry);

    std::uint32_t GetGeometryCount() const;
    std::uint64_t GetBufferAddress() const;

private:
    VKW::BufferResource* m_PersistentGeometryBuffer;
    std::uint32_t        m_PersistentGeometryNextFreeOffset;

    VKW::BufferResource* m_MainGeometryBuffer;
};

}
