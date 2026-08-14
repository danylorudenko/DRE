#include <gfx\renderer\RayTracingManager.hpp>

#include <vk_wrapper\descriptor\DescriptorManager.hpp>

#include <gfx\GraphicsManager.hpp>
#include <engine\data\Geometry.hpp>

namespace GFX
{

std::uint32_t constexpr MAX_INSTACES_IN_TLAS        = 1024 * 8;
std::uint32_t constexpr BLAS_SCRATCH_BUFFER_SIZE    = 1024 * 1024 * 64;

RayTracingManager::RayTracingManager(VKW::Device* device, GlobalGeometry* globalGeometry)
    : DeviceChild{ device }
    , m_GlobalGeometryManager{ globalGeometry }
    , m_BLASTable{ &DRE::g_PersistentDataAllocator }
    , m_MainSceneTLAS{}
    , m_ScratchBuffer{}
    , m_ScratchAlignment{ m_ParentDevice->GetLogicalDevice()->Properties().accelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment }
{
    static_assert(sizeof(void*) == sizeof(DRE::U64), "Will build and run only on 64-bit systems");

    m_InstanceInputBuffer = [device]() {
        return device->GetResourcesController()->CreateBuffer(
            sizeof(VkAccelerationStructureInstanceKHR) * MAX_INSTACES_IN_TLAS,
            VKW::BufferUsage::ACCELERATION_STRUCTURE_INPUT,
            "AS_build_instances"
        );
    };

    m_ScratchBuffer = [device]() {
        return device->GetResourcesController()->CreateBuffer(
            BLAS_SCRATCH_BUFFER_SIZE,
            VKW::BufferUsage::STORAGE,
            "AS_build_scratch"
        );
    };

    m_MainSceneTLAS = []() {
        return TLAS{
            .m_LogicalHandle = nullptr,
            .m_ResidenceBuffer = nullptr
        };
    };
}

RayTracingManager::TLAS* RayTracingManager::Update(GFX::FrameID frameID, RenderView const& view, VKW::Context& context)
{
    VKW::BufferResource* scratchBuffer = m_ScratchBuffer.Get(frameID);
    m_ScratchLinearAllocator.Reset(reinterpret_cast<void*>(scratchBuffer->gpuAddress_), scratchBuffer->size_);

    return BuildSceneAccelerationStructure(view, context);
}

RayTracingManager::BLAS* RayTracingManager::RegisterGeometry(Data::Geometry* geometry, VKW::Context& context)
{
    VKW::ImportTable* table = m_ParentDevice->GetFuncTable();

    GlobalGeometry::GeometryGPU* gpuGeometry = m_GlobalGeometryManager->FindOrUploadGeometry(geometry);
    m_GlobalGeometryManager->UpdateGPUGeometry(context);

    VkAccelerationStructureGeometryKHR geometryDesc;
    geometryDesc.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometryDesc.pNext = nullptr;
    geometryDesc.flags = VK_FLAGS_NONE;
    geometryDesc.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometryDesc.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometryDesc.geometry.triangles.pNext = nullptr;
    geometryDesc.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    geometryDesc.geometry.triangles.vertexData.deviceAddress = gpuGeometry->GetVertexGPUAddress(); 
    geometryDesc.geometry.triangles.vertexStride = sizeof(Data::DREVertex);
    geometryDesc.geometry.triangles.maxVertex = gpuGeometry->GetVertexCount() - 1; // yep, -1 is according to the spec
    geometryDesc.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    geometryDesc.geometry.triangles.indexData.deviceAddress = gpuGeometry->GetIndexGPUAddress();
    geometryDesc.geometry.triangles.transformData.deviceAddress = 0;

    DRE::U32 const primCount = gpuGeometry->GetIndexCount() / 3;


    VkAccelerationStructureBuildGeometryInfoKHR buildInfo;
    buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.pNext = nullptr;
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    buildInfo.flags = VK_FLAGS_NONE;
    buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.srcAccelerationStructure = VK_NULL_HANDLE; // needed only for update
    buildInfo.dstAccelerationStructure = VK_NULL_HANDLE; // probably need it only for build, now we just get sizes
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &geometryDesc;
    buildInfo.ppGeometries = nullptr;

    VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo;
    buildSizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    buildSizeInfo.pNext = nullptr;

    table->vkGetAccelerationStructureBuildSizesKHR(
        m_ParentDevice->GetLogicalDevice()->Handle(),
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &buildInfo,
        &primCount,
        &buildSizeInfo);

    // create residence buffer
    // create AS
    // alloc scratch
    // cmdBuild
    VKW::BufferResource* asBuffer = m_ParentDevice->GetResourcesController()->CreateBuffer(buildSizeInfo.accelerationStructureSize, VKW::BufferUsage::ACCELERATION_STRUCTURE, "blas_buf");
    VKW::AccelerationStructureResource* vkBLAS = m_ParentDevice->GetResourcesController()->CreateBLAS(asBuffer, "blas");
    DRE::U64 scratchAddress = reinterpret_cast<DRE::U64>(m_ScratchLinearAllocator.Alloc(buildSizeInfo.buildScratchSize, m_ScratchAlignment));

    context.CmdBuildBLAS(
        vkBLAS, scratchAddress,
        gpuGeometry->GetVertexGPUAddress(), sizeof(Data::DREVertex), gpuGeometry->GetVertexCount(),
        gpuGeometry->GetIndexGPUAddress(), gpuGeometry->GetIndexCount());

    context.FlushAll();
    context.WaitIdle();
    m_ScratchLinearAllocator.Reset();

    BLAS blas;
    blas.m_LogicalHandle = vkBLAS;
    blas.m_ResidenceBuffer = asBuffer;
    blas.m_ReferenceGeometry = geometry;

    context.CmdMemoryDependency(
        VKW::RESOURCE_ACCESS_ACCELERATION_STRUCTURE_BUILD, VKW::STAGE_AS_BUILD,
        VKW::RESOURCE_ACCESS_ACCELERATION_STRUCTURE_TRACE, VKW::STAGE_RAY_TRACE
    );

    return &m_BLASTable.Emplace(geometry, blas);
}

void RayTracingManager::UnregisterGeometry(BLAS* blas)
{
    DRE_ASSERT(false, "We don't do that here (unimplemented)");
}

RayTracingManager::TLAS* RayTracingManager::BuildSceneAccelerationStructure(RenderView const& view, VKW::Context& context)
{
    auto frameID = g_GraphicsManager->GetCurrentFrameID();
    VkAccelerationStructureInstanceKHR* instancesStart = reinterpret_cast<VkAccelerationStructureInstanceKHR*>(m_InstanceInputBuffer.Get(frameID)->memory_.GetRegionMappedPtr());

    auto& renderableObjects = view.GetObjects();
    if (renderableObjects.Empty())
    {
        return nullptr;
    }

    for (DRE::U32 i = 0, size = renderableObjects.Size(); i < size; i++)
    {
        glm::mat4 const& m = renderableObjects[i]->GetInstanceGPU().GetTransform();
        instancesStart[i].transform = {
             m[0][0], m[1][0], m[2][0], m[3][0],
             m[0][1], m[1][1], m[2][1], m[3][0],
             m[0][2], m[1][2], m[2][2], m[3][0]
        };
        instancesStart[i].instanceCustomIndex = renderableObjects[i]->GetInstanceGPU().GetID();
        instancesStart[i].mask = 0xFFFFFFFF;
        instancesStart[i].instanceShaderBindingTableRecordOffset = 0; // hmm
        instancesStart[i].flags = 0 /*VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR*/;
        instancesStart[i].accelerationStructureReference = renderableObjects[i]->GetBLASResource()->acAddress_;
    }


    VkAccelerationStructureGeometryKHR geometry;
    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.pNext = nullptr;
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry.geometry.instances.pNext = nullptr;
    geometry.geometry.instances.arrayOfPointers = VK_FALSE;
    geometry.geometry.instances.data.deviceAddress = m_InstanceInputBuffer.Get(frameID)->gpuAddress_;
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo;
    buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.pNext = nullptr;
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
    buildInfo.dstAccelerationStructure = VK_NULL_HANDLE; // to fill after GetSizes and creation
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &geometry;
    buildInfo.ppGeometries = nullptr;
    buildInfo.scratchData.deviceAddress = 0; // to fill after GetSizes

    // get sizes
    VkAccelerationStructureBuildSizesInfoKHR sizes;
    sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    sizes.pNext = nullptr;
    sizes.accelerationStructureSize = 0;
    sizes.buildScratchSize = 0;
    sizes.updateScratchSize = 0;

    DRE::U32 primitiveCount = renderableObjects.Size();
    m_ParentDevice->GetFuncTable()->vkGetAccelerationStructureBuildSizesKHR(m_ParentDevice->GetLogicalDevice()->Handle(),
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &buildInfo,
        &primitiveCount,
        &sizes);

    // free old TLAS
    VKW::ResourcesController* resourcesController = m_ParentDevice->GetResourcesController();

    TLAS& currentTLAS = m_MainSceneTLAS.Get(g_GraphicsManager->GetCurrentFrameID());
    if (currentTLAS.m_LogicalHandle != VK_NULL_HANDLE)
    {
        resourcesController->FreeAccelerationStructure(currentTLAS.m_LogicalHandle);
        resourcesController->FreeBuffer(currentTLAS.m_ResidenceBuffer);
    }

    // create TLAS
    VKW::BufferResource* tlasBuffer = m_ParentDevice->GetResourcesController()->CreateBuffer(sizes.accelerationStructureSize, VKW::BufferUsage::ACCELERATION_STRUCTURE, "TLAS_buffer");
    VKW::AccelerationStructureResource* tlas = m_ParentDevice->GetResourcesController()->CreateTLAS(tlasBuffer, "TLAS");

    // alloc scratch
    DRE::U64 scratchGPUAddress = reinterpret_cast<DRE::U64>(m_ScratchLinearAllocator.Alloc(sizes.buildScratchSize, 256));

    // build
    // context builds all desc struct anew
    context.CmdBuildTLAS(tlas, scratchGPUAddress, renderableObjects.Size(), m_InstanceInputBuffer.Get(frameID)->gpuAddress_);

    context.FlushAll();

    currentTLAS.m_LogicalHandle = tlas;
    currentTLAS.m_ResidenceBuffer = tlasBuffer;

    return &currentTLAS;
}

RayTracingManager::BLAS* RayTracingManager::GetGeometryBLAS(Data::Geometry* geometry)
{
    return m_BLASTable.Find(geometry).value;
}

RayTracingManager::~RayTracingManager()
{
    VKW::ResourcesController* resourcesController = m_ParentDevice->GetResourcesController();

    m_MainSceneTLAS.ForEach([resourcesController](auto& tlas)
    {
        resourcesController->FreeAccelerationStructure(tlas.m_LogicalHandle);
        resourcesController->FreeBuffer(tlas.m_ResidenceBuffer);
    });

    m_ScratchBuffer.ForEach([resourcesController](auto& buffer)
    {
        resourcesController->FreeBuffer(buffer);
    });

    m_BLASTable.ForEach([resourcesController](auto& pair)
    {
        BLAS* blas = pair.value;
        resourcesController->FreeAccelerationStructure(blas->m_LogicalHandle);
        resourcesController->FreeBuffer(blas->m_ResidenceBuffer);
    });

    m_InstanceInputBuffer.ForEach([resourcesController](auto& buffer)
    {
        resourcesController->FreeBuffer(buffer);
    });
}



}