#pragma once

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>
#include <foundation\container\HashTable.hpp>
#include <foundation\memory\AllocatorLinear.hpp>

#include <vk_wrapper\resources\Resource.hpp>
#include <gfx\buffer\StorageBuffer.hpp>
#include <gfx\renderer\GlobalGeometryManager.hpp>


namespace VKW
{
class Context;
class DescriptorManager;
}

namespace Data
{
class Geometry;
}

namespace WORLD
{
class Scene;
class SceneNode;
class Entity;
}

namespace GFX
{

class RenderView;

/////////////////////////////
class RayTracingManager
    : public NonMovable
    , public DeviceChild
{
public:
    struct BLAS
    {
        VKW::AccelerationStructureResource* m_LogicalHandle     = nullptr;
        VKW::BufferResource*                m_ResidenceBuffer   = nullptr;
        Data::Geometry*                     m_ReferenceGeometry = nullptr;
    };

    struct TLAS
    {
        VKW::AccelerationStructureResource* m_LogicalHandle     = nullptr;
        VKW::BufferResource*                m_ResidenceBuffer   = nullptr;
    };

    RayTracingManager(VKW::Device* device, GlobalGeometry* globalGeometry);

    BLAS* RegisterGeometry(Data::Geometry* geometry, VKW::Context& context);
    void UnregisterGeometry(BLAS* blas);

    TLAS* BuildSceneAccelerationStructure(RenderView const& view, VKW::Context& context);

    BLAS* GetGeometryBLAS(Data::Geometry* geometry);
    TLAS* GetMainSceneTLAS() { return &m_MainSceneTLAS; }

    ~RayTracingManager();

private:
    GlobalGeometry* m_GlobalGeometryManager;

    DRE::HashTable<Data::Geometry*, BLAS, DRE::AllocatorLinear> m_BLASTable;
    TLAS m_MainSceneTLAS;

    VKW::BufferResource* m_InstanceInputBuffer;
    VKW::BufferResource* m_ScratchBuffer;
    DRE::AllocatorLinear m_ScratchLinearAllocator;
    std::uint32_t        m_ScratchAlignment;
};

}

