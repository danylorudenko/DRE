#include <gfx\pass\DDGI.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>

#include <engine\data\GeometryLibrary.hpp>
#include <engine\ApplicationContext.hpp>

#include <common\global_illumination\ddgi_common.slang>

namespace GFX
{

DDGI::DDGI()
    : m_ProbeDebugSphereGPU{ nullptr }
{
}

void DDGI::Initialize(Data::GeometryLibrary* geometryLibrary)
{
    Data::Geometry* ddgiProbeGeometry = geometryLibrary->GetGeometry(DDGI::GetProbeDebugSphereGeometryName());
    m_ProbeDebugSphereGPU = g_GraphicsManager->GetGlobalGeometryManager().FindOrUploadGeometry(ddgiProbeGeometry);
}

glm::uvec3 DDGI::GetProbeCount3D() const
{
    GraphicsSettings& settings = g_GraphicsManager->GetGraphicsSettings();
    return glm::uvec3(settings.m_DDGIProbeCountX, settings.m_DDGIProbeCountY, settings.m_DDGIProbeCountZ);
}

DRE::U32 DDGI::GetProbeTotalCount() const
{
    glm::uvec3 const ddgiProbeGridDimentions = GetProbeCount3D();
    return ddgiProbeGridDimentions.x * ddgiProbeGridDimentions.y * ddgiProbeGridDimentions.z;
}

DRE::U32 DDGI::GetProbeDataBufferSize() const
{
    return sizeof(DDGIProbeEncodedData) * GetProbeTotalCount();
}

DRE::U32 DDGI::GetProbeLUTBufferSize() const
{
    return sizeof(DRE::U32) * GetProbeTotalCount();
}

DRE::U32 DDGI::GetTracedProbeCount() const
{
    // only the most important half of the probes is updated each frame
    return (GetProbeTotalCount() + 1) / 2;
}

DRE::U32 DDGI::GetProbeResolutionIrradiance() const
{
    return DDGI_IRRADIANCE_PROBE_RESOLUTION;
}

glm::uvec2 DDGI::GetProbeAtlasIrradianceDimentions() const
{
    glm::uvec3 const probeCounts = GetProbeCount3D();
    glm::uvec2 atlasResolution = glm::uvec2(probeCounts) * GetProbeResolutionIrradiance();
    atlasResolution.x *= probeCounts.z;

    return atlasResolution;
}

DRE::U32 DDGI::GetProbeResolutionVisibility() const
{
    return DDGI_VISIBILITY_PROBE_RESOLUTION;
}

glm::uvec2 DDGI::GetProbeAtlasVisibilityDimentions() const
{
    glm::uvec3 const probeCounts = GetProbeCount3D();
    glm::uvec2 atlasResolution = glm::uvec2(probeCounts) * GetProbeResolutionVisibility();
    atlasResolution.x *= probeCounts.z;

    return atlasResolution;
}

DDGIConstantBuffer DDGI::GetConstantBuffer(RenderGraph& graph) const
{
    auto& ddgiDebugState = DRE::g_AppContext.m_DDGIDebugState;
    auto& settings = g_GraphicsManager->GetGraphicsSettings();

    glm::uvec3 const ddgiProbeGridDimentions = GetProbeCount3D();
    glm::vec3 const ddgiProbeWorldDistance = glm::vec3(settings.m_DDGIProbeWorldDistance);

    DDGIConstantBuffer cb{};
    cb.probeGridDimentions = ddgiProbeGridDimentions;
    cb.probeDebugScale = ddgiDebugState.m_SphereScale;

    cb.ddgiProbeData = reinterpret_cast<DDGIProbeEncodedData*>(graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeData))->GetResource()->gpuAddress_);
    cb.totalProbeCount = GetProbeTotalCount();
    cb.tracedProbeCount = GetTracedProbeCount();

    cb.probesWorldDistance = glm::vec4(ddgiProbeWorldDistance, glm::max(ddgiProbeWorldDistance.x, glm::max(ddgiProbeWorldDistance.y, ddgiProbeWorldDistance.z)));

    cb.probeWorldOffset = settings.m_DDGIProbeWorldOffset;
    cb.rayCountPerProbe = settings.m_DDGIRayPerProbeCount;

    cb.probeGeometryVertexCount = m_ProbeDebugSphereGPU->GetVertexCount();
    cb.probeGeometryIndexCount = m_ProbeDebugSphereGPU->GetIndexCount();
    cb.probeIrradianceResolution = GetProbeResolutionIrradiance();
    cb.probeVisibilityResolution = GetProbeResolutionVisibility();

    cb.irradianceAtlasTextureID = graph.GetTemporalTextureCurrent(RESOURCE_ID(TextureID::DDGI_ProbeIrradiance))->GetShaderGlobalDescriptor().id_;
    cb.irradianceHistoryTextureID = graph.GetTemporalTextureHistory(RESOURCE_ID(TextureID::DDGI_ProbeIrradiance))->GetShaderGlobalDescriptor().id_;
    cb.visibilityAtlasTextureID = graph.GetTemporalTextureCurrent(RESOURCE_ID(TextureID::DDGI_AtlasVisibility))->GetShaderGlobalDescriptor().id_;
    cb.visibilityHistoryTextureID = graph.GetTemporalTextureHistory(RESOURCE_ID(TextureID::DDGI_AtlasVisibility))->GetShaderGlobalDescriptor().id_;

    cb.updateRate = settings.m_DDGIUpdateRate;
    cb.rayDistributionMode = settings.m_DDGIRayDistributionMode;
    cb.visualizationMode = static_cast<DRE::U32>(ddgiDebugState.m_VisMode);
    cb.drawSelectedCage = ddgiDebugState.m_DrawSelectedCage ? 1u : 0u;

    cb.debugProbeFocus = ddgiDebugState.m_FocusProbe;
    cb.debugDrawRays = ddgiDebugState.m_DrawProbeRays;

    cb.debugHighlightProbeTile = ddgiDebugState.m_HighlightProbeTile ? 1u : 0u;

    return cb;
}


////////////////////////////////////////////////
PassID DDGIProbePrimePass::GetID() const
{
    return PassID::DDGIProbePrime;
}

void DDGIProbePrimePass::RegisterResources(RenderGraph& graph)
{
    DDGI& ddgi = g_GraphicsManager->GetDDGI();
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeData), ddgi.GetProbeDataBufferSize(), VKW::RESOURCE_ACCESS_SHADER_WRITE);
}

void DDGIProbePrimePass::Render(RenderGraph& graph, VKW::Context& context)
{
    static bool probeDataPrimed = false;
    if (probeDataPrimed)
        return;

    DRE_GPU_SCOPE(DDGIProbePrime);

    StorageBuffer* ddgiProbeData = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeData));

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeData->GetResource(), VKW::RESOURCE_ACCESS_GENERIC_WRITE, VKW::STAGE_COMPUTE);

    PipelineEntry* entry = g_GraphicsManager->GetPipelineDB().GetEntry("ddgi_probe_prime");
    UniformProxy uniform = graph.AllocateUniform(GetID(), context, sizeof(DDGIConstantBuffer));

    uniform.WriteMember140(g_GraphicsManager->GetDDGI().GetConstantBuffer(graph));
    uniform.FlushWrites();

    context.CmdBindComputePipeline(entry->GetPipeline());
    ResourceBinder binder = g_GraphicsManager->CreateResourceBinder(entry, 0);
    binder.AddUniform(0, &uniform);
    binder.AddStorageBuffer(1, ddgiProbeData);
    binder.FlushDescriptorWrites();

    context.CmdBindComputeDescriptorSets(entry->GetLayout(), binder.GetTargetSetID(), 1, &binder.GetDescriptorSet());

    glm::uvec3 const ddgiProbeDimentions = g_GraphicsManager->GetDDGI().GetProbeCount3D();

    glm::uvec3 const dispatchSize = VKW::CalculateDispatchSize(entry->GetPipeline()->GetWorkgroupSize(), ddgiProbeDimentions);
    context.CmdDispatch(dispatchSize.x, dispatchSize.y, dispatchSize.z);

    probeDataPrimed = true;
}


////////////////////////////////////////////////

constexpr DRE::U32 C_DDGI_SORT_REGION_SIZE = 1024;

PassID DDGIProbeSortPass::GetID() const
{
    return PassID::DDGIProbeSort;
}

void DDGIProbeSortPass::RegisterResources(RenderGraph& graph)
{
    DDGI& ddgi = g_GraphicsManager->GetDDGI();

    DRE::U32 const sortGroupCount = (ddgi.GetProbeTotalCount() + C_DDGI_SORT_REGION_SIZE - 1) / C_DDGI_SORT_REGION_SIZE;

    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeData), ddgi.GetProbeDataBufferSize(), VKW::RESOURCE_ACCESS_SHADER_READ);
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeSortedIndices), ddgi.GetProbeLUTBufferSize(), VKW::RESOURCE_ACCESS_SHADER_RW, GraphResourceFlags::PING_PONG);
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeSortCounts), sizeof(BucketCounts) * sortGroupCount, VKW::RESOURCE_ACCESS_SHADER_RW);
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeSortGroupPrefixSums), sizeof(PrefixSums) * sortGroupCount, VKW::RESOURCE_ACCESS_SHADER_RW);
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeSortGlobalPrefixSum), sizeof(PrefixSums), VKW::RESOURCE_ACCESS_SHADER_WRITE);
}

void DDGIProbeSortPass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(DDGIProbeSort);

    DDGI& ddgi = g_GraphicsManager->GetDDGI();

    GraphResourcesManager::GraphBuffer* sortedIndices = graph.GetPingPongBuffer(RESOURCE_ID(BufferID::DDGI_ProbeSortedIndices));

    StorageBuffer* ddgiProbeData            = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeData));
    StorageBuffer* groupBucketCounts        = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeSortCounts));
    StorageBuffer* groupPrefixSums          = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeSortGroupPrefixSums));
    StorageBuffer* globalPrefixSum          = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeSortGlobalPrefixSum));

    DependencyManager& dependencyManager = g_GraphicsManager->GetDependencyManager();

    UniformProxy uniform = graph.AllocateUniform(GetID(), context, sizeof(DDGIConstantBuffer));
    uniform.WriteMember140(ddgi.GetConstantBuffer(graph));
    uniform.FlushWrites();

    // probe data itself is never reordered, only indices into it are sorted
    dependencyManager.ResourceBarrier(context, ddgiProbeData->GetResource(), VKW::RESOURCE_ACCESS_SHADER_READ, VKW::STAGE_COMPUTE);

    PipelineEntry* initEntry = g_GraphicsManager->GetPipelineDB().GetEntry("ddgi_probe_sort_init");
    {
        DRE_GPU_SCOPE(DDGIProbeSortInit);

        StorageBuffer& indicesInit = sortedIndices->multiStorage[sortedIndices->pingPongStart % 2];

        dependencyManager.ResourceBarrier(context, indicesInit.GetResource(), VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);

        ResourceBinder binder = g_GraphicsManager->CreateResourceBinder(initEntry, 0);
        binder.AddStorageBuffer(0, &indicesInit);
        binder.AddUniform(1, &uniform);
        binder.FlushDescriptorWrites();

        context.CmdBindComputePipeline(initEntry->GetPipeline());
        context.CmdBindComputeDescriptorSets(initEntry->GetLayout(), binder.GetTargetSetID(), 1, &binder.GetDescriptorSet());

        glm::uvec3 const threadDims{ ddgi.GetProbeTotalCount(), 1, 1 };
        glm::uvec3 const dims = VKW::CalculateDispatchSize(initEntry->GetPipeline()->GetWorkgroupSize(), threadDims);
        context.CmdDispatch(dims.x, dims.y, dims.z);
    }

    DRE::U32 const regionSizePerGroup = C_DDGI_SORT_REGION_SIZE;

    struct PushConstant
    {
        DRE::U32 sortingPass; // 4 bits per pass
        DRE::U32 regionSizePerGroup;
    } pushConstant;
    pushConstant.regionSizePerGroup = regionSizePerGroup;

    DRE::U32 const sortPassCount = 32 / 4;

    for (DRE::U32 sortPass = 0; sortPass < sortPassCount; sortPass++)
    {
        DRE_GPU_SCOPE(DDGIProbeSortPass);
        pushConstant.sortingPass = sortPass;

        StorageBuffer& indicesInput = sortedIndices->multiStorage[sortedIndices->pingPongStart % 2];
        StorageBuffer& indicesOutput = sortedIndices->multiStorage[(sortedIndices->pingPongStart + 1) % 2];

        PipelineEntry* countEntry = g_GraphicsManager->GetPipelineDB().GetEntry("ddgi_probe_sort_count");
        {
            DRE_GPU_SCOPE(DDGIProbeSortPassCount);

            dependencyManager.ResourceBarrier(context, indicesInput.GetResource(),          VKW::RESOURCE_ACCESS_SHADER_READ,  VKW::STAGE_COMPUTE);
            dependencyManager.ResourceBarrier(context, groupBucketCounts->GetResource(),    VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);

            ResourceBinder binder = g_GraphicsManager->CreateResourceBinder(countEntry, 0);
            binder.AddStorageBuffer(0, &indicesInput);
            binder.AddStorageBuffer(1, groupBucketCounts);
            binder.AddUniform(2, &uniform);
            binder.AddStorageBuffer(3, ddgiProbeData);
            binder.FlushDescriptorWrites();

            context.CmdBindComputePipeline(countEntry->GetPipeline());
            context.CmdBindComputeDescriptorSets(countEntry->GetLayout(), binder.GetTargetSetID(), 1, &binder.GetDescriptorSet());
            context.CmdPushConstants(countEntry->GetLayout(), VKW::DESCRIPTOR_STAGE_ALL, 0, sizeof(PushConstant), &pushConstant);

            VKW::WorkgroupSize const& workgroupSize = countEntry->GetPipeline()->GetWorkgroupSize();
            DRE::U32 const probesPerThread = regionSizePerGroup / workgroupSize.dims[0];

            glm::uvec3 const threadDims{ (ddgi.GetProbeTotalCount() + probesPerThread-1) / probesPerThread, 1, 1 };
            glm::uvec3 const dims = VKW::CalculateDispatchSize(workgroupSize, threadDims);
            context.CmdDispatch(dims.x, dims.y, dims.z);
        }

        PipelineEntry* prefixSumsEntry = g_GraphicsManager->GetPipelineDB().GetEntry("ddgi_probe_sort_prefix_sums");
        {
            DRE_GPU_SCOPE(DDGIProbeSortPassPrefixSum);

            dependencyManager.ResourceBarrier(context, groupBucketCounts->GetResource(),    VKW::RESOURCE_ACCESS_SHADER_READ,  VKW::STAGE_COMPUTE);
            dependencyManager.ResourceBarrier(context, groupPrefixSums->GetResource(),      VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);
            dependencyManager.ResourceBarrier(context, globalPrefixSum->GetResource(),      VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);

            ResourceBinder binder = g_GraphicsManager->CreateResourceBinder(prefixSumsEntry, 0);
            binder.AddStorageBuffer(0, groupBucketCounts);
            binder.AddStorageBuffer(1, groupPrefixSums);
            binder.AddStorageBuffer(2, globalPrefixSum);
            binder.AddUniform(3, &uniform);
            binder.FlushDescriptorWrites();

            context.CmdBindComputePipeline(prefixSumsEntry->GetPipeline());
            context.CmdBindComputeDescriptorSets(prefixSumsEntry->GetLayout(), binder.GetTargetSetID(), 1, &binder.GetDescriptorSet());
            context.CmdPushConstants(prefixSumsEntry->GetLayout(), VKW::DESCRIPTOR_STAGE_ALL, 0, sizeof(PushConstant), &pushConstant);

            context.CmdDispatch(1, 1, 1);
        }

        PipelineEntry* reorderEntry = g_GraphicsManager->GetPipelineDB().GetEntry("ddgi_probe_sort_reorder");
        {
            DRE_GPU_SCOPE(DDGIProbeSortPassReorder);

            dependencyManager.ResourceBarrier(context, indicesInput.GetResource(),          VKW::RESOURCE_ACCESS_SHADER_READ,  VKW::STAGE_COMPUTE);
            dependencyManager.ResourceBarrier(context, groupPrefixSums->GetResource(),      VKW::RESOURCE_ACCESS_SHADER_READ,  VKW::STAGE_COMPUTE);
            dependencyManager.ResourceBarrier(context, globalPrefixSum->GetResource(),      VKW::RESOURCE_ACCESS_SHADER_READ,  VKW::STAGE_COMPUTE);
            dependencyManager.ResourceBarrier(context, indicesOutput.GetResource(),         VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);

            ResourceBinder binder = g_GraphicsManager->CreateResourceBinder(reorderEntry, 0);
            binder.AddStorageBuffer(0, &indicesInput);
            binder.AddStorageBuffer(1, groupPrefixSums);
            binder.AddStorageBuffer(2, globalPrefixSum);
            binder.AddStorageBuffer(3, &indicesOutput);
            binder.AddUniform(4, &uniform);
            binder.AddStorageBuffer(5, ddgiProbeData);
            binder.FlushDescriptorWrites();

            context.CmdBindComputePipeline(reorderEntry->GetPipeline());
            context.CmdBindComputeDescriptorSets(reorderEntry->GetLayout(), binder.GetTargetSetID(), 1, &binder.GetDescriptorSet());
            context.CmdPushConstants(reorderEntry->GetLayout(), VKW::DESCRIPTOR_STAGE_ALL, 0, sizeof(PushConstant), &pushConstant);

            VKW::WorkgroupSize const& workgroupSize = reorderEntry->GetPipeline()->GetWorkgroupSize();
            DRE::U32 const probesPerThread = regionSizePerGroup / workgroupSize.dims[0];

            glm::uvec3 const threadDims{ (ddgi.GetProbeTotalCount() + probesPerThread - 1) / probesPerThread, 1, 1 };
            glm::uvec3 const dims = VKW::CalculateDispatchSize(workgroupSize, threadDims);
            context.CmdDispatch(dims.x, dims.y, dims.z);
        }

        sortedIndices->pingPongStart = (sortedIndices->pingPongStart + 1) % 2;
    }

    // pingPongStart now points to the last output, so graph.GetBuffer(DDGI_ProbeSortedIndices) returns the sorted LUT down the frame
}


PassID DDGIProbeTracePass::GetID() const
{
    return PassID::DDGIProbeTrace;
}

void DDGIProbeTracePass::RegisterResources(RenderGraph& graph)
{
    DDGI& ddgi = g_GraphicsManager->GetDDGI();

    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeData), ddgi.GetProbeDataBufferSize(), VKW::RESOURCE_ACCESS_SHADER_RW);
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeSampleBuffer), sizeof(DDGIProbeSamples) * ddgi.GetProbeTotalCount(), VKW::RESOURCE_ACCESS_SHADER_WRITE);
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeSortedIndices), ddgi.GetProbeLUTBufferSize(), VKW::RESOURCE_ACCESS_SHADER_READ, GraphResourceFlags::PING_PONG);
}

void DDGIProbeTracePass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(DDGIProbeTrace);

    DDGI& ddgi = g_GraphicsManager->GetDDGI();

    StorageBuffer* ddgiProbeData = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeData));
    StorageBuffer* ddgiProbeSampleBuffer = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeSampleBuffer));
    StorageBuffer* sortedIndices = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeSortedIndices));

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeData->GetResource(), VKW::RESOURCE_ACCESS_SHADER_RW, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeSampleBuffer->GetResource(), VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, sortedIndices->GetResource(), VKW::RESOURCE_ACCESS_SHADER_READ, VKW::STAGE_COMPUTE);

    UniformProxy uniform = graph.AllocateUniform(GetID(), context, sizeof(DDGIConstantBuffer));
    uniform.WriteMember140(ddgi.GetConstantBuffer(graph));
    uniform.FlushWrites();

    PipelineEntry* entry = g_GraphicsManager->GetPipelineDB().GetEntry("ddgi_probe_trace");
    ResourceBinder binder = g_GraphicsManager->CreateResourceBinder(entry, 0);
    binder.AddUniform(0, &uniform);
    binder.AddStorageBuffer(1, ddgiProbeSampleBuffer);
    binder.AddStorageBuffer(2, ddgiProbeData);
    binder.AddStorageBuffer(3, sortedIndices);
    binder.FlushDescriptorWrites();

    context.CmdBindComputePipeline(entry->GetPipeline());
    context.CmdBindComputeDescriptorSets(entry->GetLayout(), binder.GetTargetSetID(), 1, &binder.GetDescriptorSet());

    // one thread per traced probe, probe index is looked up in the sorted LUT
    glm::uvec3 const threadDims{ ddgi.GetTracedProbeCount(), 1, 1 };
    glm::uvec3 const dispatchSize = VKW::CalculateDispatchSize(entry->GetPipeline()->GetWorkgroupSize(), threadDims);
    context.CmdDispatch(dispatchSize.x, dispatchSize.y, dispatchSize.z);
}



////////////////////////////////////////////////
PassID DDGIProbeLightingPass::GetID() const
{
    return PassID::DDGIProbeLighting;
}

void DDGIProbeLightingPass::RegisterResources(RenderGraph& graph)
{
    DDGI& ddgi = g_GraphicsManager->GetDDGI();
    glm::uvec2 const irradianceDims = ddgi.GetProbeAtlasIrradianceDimentions();

    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeSampleBuffer), sizeof(DDGIProbeSamples) * ddgi.GetProbeTotalCount(), VKW::RESOURCE_ACCESS_SHADER_READ);
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::DDGI_ProbeIrradiance), VKW::FORMAT_R16G16B16A16_FLOAT, irradianceDims.x, irradianceDims.y, VKW::RESOURCE_ACCESS_SHADER_WRITE, GraphResourceFlags::TEMPORAL | GraphResourceFlags::INIT_CLEAR_ZERO);
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeData), ddgi.GetProbeDataBufferSize(), VKW::RESOURCE_ACCESS_SHADER_READ);
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeSortedIndices), ddgi.GetProbeLUTBufferSize(), VKW::RESOURCE_ACCESS_SHADER_READ, GraphResourceFlags::PING_PONG);
}

void DDGIProbeLightingPass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(DDGIProbeLighting);

    DDGI& ddgi = g_GraphicsManager->GetDDGI();

    StorageBuffer* ddgiProbeSampleBuffer = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeSampleBuffer));
    StorageBuffer* probeData  = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeData));
    Texture* atlasIrradiance  = graph.GetTemporalTextureCurrent(RESOURCE_ID(TextureID::DDGI_ProbeIrradiance));
    Texture* atlasIrradianceHistory = graph.GetTemporalTextureHistory(RESOURCE_ID(TextureID::DDGI_ProbeIrradiance));
    StorageBuffer* sortedIndices = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeSortedIndices));

    auto& dependencyManager = g_GraphicsManager->GetDependencyManager();

    dependencyManager.ResourceBarrier(context, ddgiProbeSampleBuffer->GetResource(),    VKW::RESOURCE_ACCESS_SHADER_READ,   VKW::STAGE_COMPUTE);
    dependencyManager.ResourceBarrier(context, atlasIrradiance->GetResource(),          VKW::RESOURCE_ACCESS_SHADER_WRITE,  VKW::STAGE_COMPUTE);
    dependencyManager.ResourceBarrier(context, atlasIrradianceHistory->GetResource(),   VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);
    dependencyManager.ResourceBarrier(context, probeData->GetResource(),                VKW::RESOURCE_ACCESS_SHADER_READ,   VKW::STAGE_COMPUTE);
    dependencyManager.ResourceBarrier(context, sortedIndices->GetResource(),            VKW::RESOURCE_ACCESS_SHADER_READ,   VKW::STAGE_COMPUTE);

    UniformProxy uniform = graph.AllocateUniform(GetID(), context, sizeof(DDGIConstantBuffer));
    uniform.WriteMember140(ddgi.GetConstantBuffer(graph));
    uniform.FlushWrites();

    PipelineEntry* entry = g_GraphicsManager->GetPipelineDB().GetEntry("ddgi_probe_lighting");
    ResourceBinder binder = g_GraphicsManager->CreateResourceBinder(entry, 0);
    binder.AddUniform(0, &uniform);
    binder.AddStorageBuffer(1, ddgiProbeSampleBuffer);
    binder.AddStorageTexture(2, atlasIrradiance);
    binder.AddSampledTexture(3, atlasIrradianceHistory);
    binder.AddStorageBuffer(4, probeData);
    binder.AddStorageBuffer(5, sortedIndices);
    binder.FlushDescriptorWrites();

    context.CmdBindComputePipeline(entry->GetPipeline());
    context.CmdBindComputeDescriptorSets(entry->GetLayout(), binder.GetTargetSetID(), 1, &binder.GetDescriptorSet());

    // x - sorted probe slot, yz - 8x8 tiles inside the probe
    DRE::U32 const probeCount = ddgi.GetProbeTotalCount();
    DRE_ASSERT(probeCount <= 65535, "DDGI probe count exceeds guaranteed max workgroup count.");

    DRE::U32 const resolution = ddgi.GetProbeResolutionIrradiance();
    VKW::WorkgroupSize const& workgroupSize = entry->GetPipeline()->GetWorkgroupSize();
    glm::uvec3 const tiles = VKW::CalculateDispatchSize(workgroupSize, glm::uvec3(workgroupSize.dims[0], resolution, resolution));
    context.CmdDispatch(probeCount, tiles.y, tiles.z);
}



////////////////////////////////////////////////
PassID DDGIProbeVisibilityPass::GetID() const
{
    return PassID::DDGIProbeVisibility;
}

void DDGIProbeVisibilityPass::RegisterResources(RenderGraph& graph)
{
    DDGI& ddgi = g_GraphicsManager->GetDDGI();
    glm::uvec2 const visibilityDims = ddgi.GetProbeAtlasVisibilityDimentions();

    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeSampleBuffer), sizeof(DDGIProbeSamples) * ddgi.GetProbeTotalCount(), VKW::RESOURCE_ACCESS_SHADER_READ);
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeData), ddgi.GetProbeDataBufferSize(), VKW::RESOURCE_ACCESS_SHADER_READ);
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeSortedIndices), ddgi.GetProbeLUTBufferSize(), VKW::RESOURCE_ACCESS_SHADER_READ, GraphResourceFlags::PING_PONG);
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::DDGI_AtlasVisibility), VKW::FORMAT_R16G16_FLOAT, visibilityDims.x, visibilityDims.y, VKW::RESOURCE_ACCESS_SHADER_WRITE, GraphResourceFlags::INIT_CLEAR_ZERO | GraphResourceFlags::TEMPORAL);
}

void DDGIProbeVisibilityPass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(DDGIProbeVisibility);

    DDGI& ddgi = g_GraphicsManager->GetDDGI();

    StorageBuffer* ddgiProbeSampleBuffer = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeSampleBuffer));
    StorageBuffer* probeData  = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeData));
    Texture* atlasVisibility  = graph.GetTemporalTextureCurrent(RESOURCE_ID(TextureID::DDGI_AtlasVisibility));
    Texture* atlasVisibilityHistory = graph.GetTemporalTextureHistory(RESOURCE_ID(TextureID::DDGI_AtlasVisibility));
    StorageBuffer* sortedIndices = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeSortedIndices));

    DependencyManager& dependencyManager = g_GraphicsManager->GetDependencyManager();

    dependencyManager.ResourceBarrier(context, ddgiProbeSampleBuffer->GetResource(),    VKW::RESOURCE_ACCESS_SHADER_READ,   VKW::STAGE_COMPUTE);
    dependencyManager.ResourceBarrier(context, probeData->GetResource(),                VKW::RESOURCE_ACCESS_SHADER_READ,   VKW::STAGE_COMPUTE);
    dependencyManager.ResourceBarrier(context, sortedIndices->GetResource(),            VKW::RESOURCE_ACCESS_SHADER_READ,   VKW::STAGE_COMPUTE);
    dependencyManager.ResourceBarrier(context, atlasVisibility->GetResource(),          VKW::RESOURCE_ACCESS_SHADER_WRITE,  VKW::STAGE_COMPUTE);
    dependencyManager.ResourceBarrier(context, atlasVisibilityHistory->GetResource(),   VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);

    UniformProxy uniform = graph.AllocateUniform(GetID(), context, sizeof(DDGIConstantBuffer));
    uniform.WriteMember140(ddgi.GetConstantBuffer(graph));
    uniform.FlushWrites();

    PipelineEntry* entry = g_GraphicsManager->GetPipelineDB().GetEntry("ddgi_probe_visibility");
    ResourceBinder binder = g_GraphicsManager->CreateResourceBinder(entry, 0);
    binder.AddUniform(0, &uniform);
    binder.AddStorageBuffer(1, ddgiProbeSampleBuffer);
    binder.AddStorageTexture(2, atlasVisibility);
    binder.AddSampledTexture(3, atlasVisibilityHistory);
    binder.AddStorageBuffer(4, probeData);
    binder.AddStorageBuffer(5, sortedIndices);
    binder.FlushDescriptorWrites();

    context.CmdBindComputePipeline(entry->GetPipeline());
    context.CmdBindComputeDescriptorSets(entry->GetLayout(), binder.GetTargetSetID(), 1, &binder.GetDescriptorSet());

    // x - sorted probe slot, yz - 8x8 tiles inside the probe
    DRE::U32 const probeCount = ddgi.GetProbeTotalCount();
    DRE_ASSERT(probeCount <= 65535, "DDGI probe count exceeds guaranteed max workgroup count.");

    DRE::U32 const resolution = ddgi.GetProbeResolutionVisibility();
    VKW::WorkgroupSize const& workgroupSize = entry->GetPipeline()->GetWorkgroupSize();
    glm::uvec3 const tiles = VKW::CalculateDispatchSize(workgroupSize, glm::uvec3(workgroupSize.dims[0], resolution, resolution));
    context.CmdDispatch(probeCount, tiles.y, tiles.z);
}


////////////////////////////////////////////////
PassID DDGIProbeUpdatePass::GetID() const
{
    return PassID::DDGIProbeUpdate;
}

void DDGIProbeUpdatePass::RegisterResources(RenderGraph& graph)
{
    DDGI& ddgi = g_GraphicsManager->GetDDGI();
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeData), ddgi.GetProbeDataBufferSize(), VKW::RESOURCE_ACCESS_SHADER_RW);
}

void DDGIProbeUpdatePass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(DDGIProbeUpdate);

    DDGI& ddgi = g_GraphicsManager->GetDDGI();

    StorageBuffer* ddgiProbeData = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeData));

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeData->GetResource(), VKW::RESOURCE_ACCESS_SHADER_RW, VKW::STAGE_COMPUTE);

    UniformProxy uniform = graph.AllocateUniform(GetID(), context, sizeof(DDGIConstantBuffer));
    uniform.WriteMember140(ddgi.GetConstantBuffer(graph));
    uniform.FlushWrites();

    PipelineEntry* entry = g_GraphicsManager->GetPipelineDB().GetEntry("ddgi_probe_update");
    ResourceBinder binder = g_GraphicsManager->CreateResourceBinder(entry, 0);
    binder.AddUniform(0, &uniform);
    binder.AddStorageBuffer(1, ddgiProbeData);
    binder.FlushDescriptorWrites();

    context.CmdBindComputePipeline(entry->GetPipeline());
    context.CmdBindComputeDescriptorSets(entry->GetLayout(), binder.GetTargetSetID(), 1, &binder.GetDescriptorSet());

    glm::uvec3 const dispatchSize = VKW::CalculateDispatchSize(entry->GetPipeline()->GetWorkgroupSize(), ddgi.GetProbeCount3D());
    context.CmdDispatch(dispatchSize.x, dispatchSize.y, dispatchSize.z);
}


/////////////////////////////////////////////////
PassID DebugPassDDGIProbeDisplay::GetID() const
{
    return PassID::DebugDDGIProbeDisplay;
}

void DebugPassDDGIProbeDisplay::Initialize(RenderGraph& graph)
{
}

void DebugPassDDGIProbeDisplay::RegisterResources(RenderGraph& graph)
{
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DebugPassDDGIProbeIndirectArgs), sizeof(DrawIndexedIndirectCommand), VKW::RESOURCE_ACCESS_INDIRECT_ARGS);
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeData), g_GraphicsManager->GetDDGI().GetProbeDataBufferSize(), VKW::RESOURCE_ACCESS_SHADER_READ);

    glm::uvec2 ddgiAtlasDims = g_GraphicsManager->GetDDGI().GetProbeAtlasIrradianceDimentions();
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::DDGI_ProbeIrradiance), VKW::FORMAT_R16G16B16A16_FLOAT, ddgiAtlasDims.x, ddgiAtlasDims.y, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, GraphResourceFlags::TEMPORAL | GraphResourceFlags::INIT_CLEAR_ZERO);

    DRE::U32 renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth;
    DRE::U32 renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

    graph.RegisterRenderTarget(this,
        RESOURCE_ID(TextureID::DisplayEncodedImage),
        g_GraphicsManager->GetDisplayEncodedFormat(), renderWidth, renderHeight,
        0);

    graph.RegisterDepthOnlyTarget(this,
        RESOURCE_ID(TextureID::MainDepth),
        g_GraphicsManager->GetMainDepthFormat(), renderWidth, renderHeight);
}

void DebugPassDDGIProbeDisplay::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(DebugPassDDGIProbeDisplay);

    auto& ddgiDebugState = DRE::g_AppContext.m_DDGIDebugState;
    if (!ddgiDebugState.m_DrawProbes)
        return;

    DRE::U32 renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth;
    DRE::U32 renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

    StorageBuffer* ddgiProbeIndirectArgs = graph.GetBuffer(RESOURCE_ID(BufferID::DebugPassDDGIProbeIndirectArgs));
    StorageBuffer* ddgiProbeData         = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeData));
    UniformProxy ddgiUniform = graph.AllocateUniform(GetID(), context, sizeof(DDGIConstantBuffer));

    {
        DRE_GPU_SCOPE(DebugPassDDGIProbeDisplayArgs);

        g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeIndirectArgs->GetResource(), VKW::RESOURCE_ACCESS_GENERIC_WRITE, VKW::STAGE_TRANSFER);
        context.CmdFillBuffer(ddgiProbeIndirectArgs->GetResource(), 0, sizeof(DrawIndexedIndirectCommand), 0);

        g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeIndirectArgs->GetResource(), VKW::RESOURCE_ACCESS_GENERIC_RW,   VKW::STAGE_COMPUTE);
        g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeData->GetResource(),         VKW::RESOURCE_ACCESS_GENERIC_READ, VKW::STAGE_COMPUTE);


        ddgiUniform.WriteMember140(g_GraphicsManager->GetDDGI().GetConstantBuffer(graph));
        ddgiUniform.FlushWrites();

        PipelineEntry* indirectFillEntry = g_GraphicsManager->GetPipelineDB().GetEntry("debug_view_ddgi_probes_args");
        ResourceBinder indirectBinder = g_GraphicsManager->CreateResourceBinder(indirectFillEntry, 0);
        indirectBinder.AddStorageBuffer(0, ddgiProbeIndirectArgs);
        indirectBinder.AddStorageBuffer(1, ddgiProbeData);
        indirectBinder.AddUniform(2, &ddgiUniform);
        indirectBinder.FlushDescriptorWrites();

        context.CmdBindComputeDescriptorSets(indirectFillEntry->GetLayout(), indirectBinder.GetTargetSetID(), 1, &indirectBinder.GetDescriptorSet());

        glm::uvec3 const probeDebugDispatchSize = VKW::CalculateDispatchSize(indirectFillEntry->GetPipeline()->GetWorkgroupSize(), g_GraphicsManager->GetDDGI().GetProbeCount3D());
        context.CmdBindPipeline(VKW::BindPoint::Compute, indirectFillEntry->GetPipeline());
        context.CmdDispatch(probeDebugDispatchSize.x, probeDebugDispatchSize.y, probeDebugDispatchSize.z);
    }

    {
        DRE_GPU_SCOPE(DebugPassDDGIProbeDisplayDraw);

        Texture* output = graph.GetTexture(RESOURCE_ID(TextureID::DisplayEncodedImage));
        Texture* depthBuffer = graph.GetTexture(RESOURCE_ID(TextureID::MainDepth));
        Texture* ddgiProbeIrradiance = graph.GetTemporalTextureCurrent(RESOURCE_ID(TextureID::DDGI_ProbeIrradiance));

        g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, output->GetResource(), VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
        g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, depthBuffer->GetResource(), VKW::RESOURCE_ACCESS_DEPTH_STENCIL_ATTACHMENT, VKW::STAGE_ALL_GRAPHICS);
        g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeData->GetResource(), VKW::RESOURCE_ACCESS_GENERIC_READ, VKW::STAGE_ALL_GRAPHICS);
        g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeIndirectArgs->GetResource(), VKW::RESOURCE_ACCESS_INDIRECT_ARGS, VKW::STAGE_ALL_GRAPHICS);
        g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeIrradiance->GetResource(), VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_FRAGMENT);

        PipelineEntry* drawSpheresEntry = g_GraphicsManager->GetPipelineDB().GetEntry("debug_view_ddgi_probes_draw");
        ResourceBinder drawBinder = g_GraphicsManager->CreateResourceBinder(drawSpheresEntry, 0);
        drawBinder.AddStorageBuffer(0, ddgiProbeIndirectArgs);
        drawBinder.AddStorageBuffer(1, ddgiProbeData);
        drawBinder.AddUniform(2, &ddgiUniform);
        drawBinder.FlushDescriptorWrites();

        context.CmdBindGraphicsDescriptorSets(drawSpheresEntry->GetLayout(), drawBinder.GetTargetSetID(), 1, &drawBinder.GetDescriptorSet());
        context.CmdBindGraphicsPipeline(drawSpheresEntry->GetPipeline());

        DRE::U32 constexpr attachmentsCount = 1;
        VKW::ImageResourceView* renderTargets[attachmentsCount] = {
            output->GetShaderView()
        };

        context.CmdBeginRendering(attachmentsCount, renderTargets, depthBuffer->GetShaderView(), nullptr);
        context.CmdSetViewport(attachmentsCount, 0, 0, renderWidth, renderHeight);
        context.CmdSetScissor(attachmentsCount, 0, 0, renderWidth, renderHeight);
    #ifndef DRE_COMPILE_FOR_RENDERDOC
        context.CmdSetPolygonMode(VKW::POLYGON_FILL);
    #endif // DRE_COMPILE_FOR_RENDERDOC

        GlobalGeometry::GeometryGPU* probeDebugSphereGPU = g_GraphicsManager->GetDDGI().GetSphereGeometry();
        context.CmdBindVertexBuffer(probeDebugSphereGPU->GetBuffer(), probeDebugSphereGPU->GetVertexOffset());
        context.CmdBindIndexBuffer(probeDebugSphereGPU->GetBuffer(), probeDebugSphereGPU->GetIndexOffset());
        context.CmdDrawIndexedIndirect(ddgiProbeIndirectArgs->GetResource());

        context.CmdEndRendering();
    }
}

}
