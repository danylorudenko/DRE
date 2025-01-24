#include <gfx\renderer\RayTracingManager.hpp>

#include <vk_wrapper\descriptor\DescriptorManager.hpp>

#include <gfx\GraphicsManager.hpp>
#include <engine\data\Geometry.hpp>

//#include <engine\scene\Scene.hpp>

namespace GFX
{

std::uint32_t constexpr MAX_INSTACES_IN_TLAS        = 1024 * 8;
std::uint32_t constexpr BLAS_SCRATCH_BUFFER_SIZE    = 1024 * 1024 * 64;

RayTracingManager::RayTracingManager(VKW::Device* device)
    : DeviceChild{ device }
    , m_BLASTable{ &DRE::g_PersistentDataAllocator }
    , m_ScratchBuffer{ nullptr }
{
    m_InstanceInputBuffer = device->GetResourcesController()->CreateBuffer(
        sizeof(VkAccelerationStructureInstanceKHR) * MAX_INSTACES_IN_TLAS,
        VKW::BufferUsage::ACCELERATION_STRUCTURE_INPUT,
        "AS_build_instances");

    m_ScratchBuffer = device->GetResourcesController()->CreateBuffer(
        BLAS_SCRATCH_BUFFER_SIZE,
        VKW::BufferUsage::STORAGE,
        "AS_build_scratch");

    static_assert(sizeof(void*) == sizeof(m_ScratchBuffer->gpuAddress_), "Will build and run only on 64-bit systems");
    m_ScratchLinearAllocator = DRE::AllocatorLinear(reinterpret_cast<void*>(m_ScratchBuffer->gpuAddress_), m_ScratchBuffer->size_);

    m_ScratchAlignment = m_ParentDevice->GetLogicalDevice()->Properties().accelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment;
}

RayTracingManager::BLAS* RayTracingManager::RegisterGeometry(Data::Geometry* geometry, VKW::Context& context)
{
    VKW::ImportTable* table = m_ParentDevice->GetFuncTable();

    GraphicsManager::GeometryGPU* gpuGeometry = g_GraphicsManager->FindOrLoadGPUGeometry(context, geometry);

    VkAccelerationStructureGeometryKHR geometryDesc;
    geometryDesc.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometryDesc.pNext = nullptr;
    geometryDesc.flags = VK_FLAGS_NONE;
    geometryDesc.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometryDesc.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometryDesc.geometry.triangles.pNext = nullptr;
    geometryDesc.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    geometryDesc.geometry.triangles.vertexData.deviceAddress = gpuGeometry->vertexBuffer->gpuAddress_;
    geometryDesc.geometry.triangles.vertexStride = sizeof(Data::DREVertex);
    geometryDesc.geometry.triangles.maxVertex = geometry->GetVertexCount() - 1; // yep, -1 is according to the spec
    geometryDesc.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    geometryDesc.geometry.triangles.indexData.deviceAddress = gpuGeometry->indexBuffer->gpuAddress_;
    geometryDesc.geometry.triangles.transformData.deviceAddress = 0;

    std::uint32_t primCount = geometry->GetIndexCount() / 3;


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
    std::uint64_t scratchAddress = reinterpret_cast<std::uint64_t>(m_ScratchLinearAllocator.Alloc(buildSizeInfo.buildScratchSize, m_ScratchAlignment));
    context.CmdBuildBLAS(
        vkBLAS, scratchAddress,
        gpuGeometry->vertexBuffer->gpuAddress_, sizeof(Data::DREVertex), geometry->GetVertexCount(),
        gpuGeometry->indexBuffer->gpuAddress_, geometry->GetIndexCount());

    context.FlushAll();
    context.WaitIdle();
    m_ScratchLinearAllocator.Reset();

    BLAS blas;
    blas.m_LogicalHandle = vkBLAS;
    blas.m_ResidenceBuffer = asBuffer;
    blas.m_ReferenceGeometry = geometry;

    return &m_BLASTable.Emplace(geometry, blas);
}

void RayTracingManager::UnregisterGeometry(BLAS* blas)
{
    DRE_ASSERT(false, "We don't do that here (unimplemented)");
}

RayTracingManager::TLAS* RayTracingManager::BuildSceneAccelerationStructure(RenderView const& view, VKW::Context& context)
{
    VkAccelerationStructureInstanceKHR* instancesStart = reinterpret_cast<VkAccelerationStructureInstanceKHR*>(m_InstanceInputBuffer->memory_.GetRegionMappedPtr());

    auto& renderableObjects = view.GetObjects();
    for (std::uint32_t i = 0, size = renderableObjects.Size(); i < size; i++)
    {
        instancesStart[i].transform = {
             1.0f, 0.0f, 0.0f, 0.0f,
             0.0f, 1.0f, 0.0f, 0.0f,
             0.0f, 0.0f, 1.0f, 0.0f FIX IT
        };
        instancesStart[i].instanceCustomIndex = i;
        instancesStart[i].mask = 0xFF;
        instancesStart[i].instanceShaderBindingTableRecordOffset = 0; // hmm
        instancesStart[i].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR;
        instancesStart[i].accelerationStructureReference = renderableObjects[i]->GetBLASResource()->residenceBuffer_->gpuAddress_;
    }


    VkAccelerationStructureGeometryKHR geometry;
    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.pNext = nullptr;
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry.geometry.instances.pNext = nullptr;
    geometry.geometry.instances.arrayOfPointers = VK_FALSE;
    geometry.geometry.instances.data.deviceAddress = m_InstanceInputBuffer->gpuAddress_;
    geometry.flags = VK_FLAGS_NONE;

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo;
    buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.pNext = nullptr;
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    buildInfo.flags = VK_FLAGS_NONE;
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

    std::uint32_t primitiveCount = 1;
    m_ParentDevice->GetFuncTable()->vkGetAccelerationStructureBuildSizesKHR(m_ParentDevice->GetLogicalDevice()->Handle(),
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &buildInfo,
        &primitiveCount,
        &sizes);

    // create TLAS
    VKW::BufferResource* tlasBuffer = m_ParentDevice->GetResourcesController()->CreateBuffer(sizes.accelerationStructureSize, VKW::BufferUsage::ACCELERATION_STRUCTURE, "TLAS_buffer");
    VKW::AccelerationStructureResource* tlas = m_ParentDevice->GetResourcesController()->CreateTLAS(tlasBuffer, "TLAS");

    // alloc scratch
    std::uint64_t scratchGPUAddress = reinterpret_cast<std::uint64_t>(m_ScratchLinearAllocator.Alloc(sizes.buildScratchSize, 16));

    // build
    // context builds all desc struct anew
    context.CmdBuildTLAS(tlas, scratchGPUAddress, renderableObjects.Size(), m_InstanceInputBuffer->gpuAddress_);

    context.FlushAll();
    context.WaitIdle();

    m_MainSceneTLAS.m_LogicalHandle = tlas;
    m_MainSceneTLAS.m_ResidenceBuffer = tlasBuffer;

    return &m_MainSceneTLAS;
}

RayTracingManager::BLAS* RayTracingManager::GetGeometryBLAS(Data::Geometry* geometry)
{
    return m_BLASTable.Find(geometry).value;
}

RayTracingManager::~RayTracingManager()
{
    VKW::ResourcesController* resourcesController = m_ParentDevice->GetResourcesController();

    if (m_MainSceneTLAS.m_LogicalHandle != VK_NULL_HANDLE)
    {
        resourcesController->FreeAccelerationStructure(m_MainSceneTLAS.m_LogicalHandle);
        resourcesController->FreeBuffer(m_MainSceneTLAS.m_ResidenceBuffer);
    }

    m_ParentDevice->GetResourcesController()->FreeBuffer(m_ScratchBuffer);
    m_BLASTable.ForEach([resourcesController](auto& pair)
    {
        BLAS* blas = pair.value;
        resourcesController->FreeAccelerationStructure(blas->m_LogicalHandle);
        resourcesController->FreeBuffer(blas->m_ResidenceBuffer);
    });
}



}