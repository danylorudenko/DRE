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
    return sizeof(DDGIProbeData) * GetProbeTotalCount();
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

    return cb;
}


////////////////////////////////////////////////
PassID DDGIProbeScatterPass::GetID() const
{
    return PassID::DDGIProbeScatter;
}

void DDGIProbeScatterPass::RegisterResources(RenderGraph& graph)
{
    DDGI& ddgi = g_GraphicsManager->GetDDGI();
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeData), ddgi.GetProbeDataBufferSize(), VKW::RESOURCE_ACCESS_SHADER_WRITE);
}

void DDGIProbeScatterPass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(DDGIProbeScatter);

    glm::uvec3 GROUP_SIZE{ 4, 4, 4, };

    StorageBuffer* ddgiProbeData = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeData));
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeData->GetResource(), VKW::RESOURCE_ACCESS_GENERIC_WRITE, VKW::STAGE_COMPUTE);

    PipelineEntry* entry = g_GraphicsManager->GetPipelineDB().GetEntry("ddgi_probe_scatter");
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

    glm::uvec3 dispatchSize = (ddgiProbeDimentions + GROUP_SIZE - 1u) / GROUP_SIZE;
    context.CmdDispatch(dispatchSize.x, dispatchSize.y, dispatchSize.z);
}


////////////////////////////////////////////////
PassID DDGIProbeTracePass::GetID() const
{
    return PassID::DDGIProbeTrace;
}

void DDGIProbeTracePass::RegisterResources(RenderGraph& graph)
{
    DDGI& ddgi = g_GraphicsManager->GetDDGI();

    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeData), ddgi.GetProbeDataBufferSize(), VKW::RESOURCE_ACCESS_SHADER_RW);
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeSampleBuffer), sizeof(DDGIProbeSamples) * ddgi.GetProbeTotalCount(), VKW::RESOURCE_ACCESS_SHADER_WRITE);
}

void DDGIProbeTracePass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(DDGIProbeTrace);

    DDGI& ddgi = g_GraphicsManager->GetDDGI();

    StorageBuffer* ddgiProbeData = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeData));
    StorageBuffer* ddgiProbeSampleBuffer = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeSampleBuffer));

    // this fill helps combat DEVICE_LOST error on writing samples to buffer (I think it was unitialized sample counter
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeSampleBuffer->GetResource(), VKW::RESOURCE_ACCESS_GENERIC_WRITE, VKW::STAGE_TRANSFER);
    context.CmdFillBuffer(ddgiProbeSampleBuffer->GetResource(), 0, sizeof(DDGIProbeSamples) * ddgi.GetProbeTotalCount(), 0);

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeData->GetResource(), VKW::RESOURCE_ACCESS_SHADER_RW, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeSampleBuffer->GetResource(), VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);

    UniformProxy uniform = graph.AllocateUniform(GetID(), context, sizeof(DDGIConstantBuffer));
    uniform.WriteMember140(ddgi.GetConstantBuffer(graph));
    uniform.FlushWrites();

    PipelineEntry* entry = g_GraphicsManager->GetPipelineDB().GetEntry("ddgi_probe_trace");
    ResourceBinder binder = g_GraphicsManager->CreateResourceBinder(entry, 0);
    binder.AddUniform(0, &uniform);
    binder.AddStorageBuffer(1, ddgiProbeData);
    binder.AddStorageBuffer(2, ddgiProbeSampleBuffer);
    binder.FlushDescriptorWrites();

    context.CmdBindComputePipeline(entry->GetPipeline());
    context.CmdBindComputeDescriptorSets(entry->GetLayout(), binder.GetTargetSetID(), 1, &binder.GetDescriptorSet());

    glm::uvec3 GROUP_SIZE{ 4, 4, 4, };
    glm::uvec3 const ddgiProbeDimentions = g_GraphicsManager->GetDDGI().GetProbeCount3D();

    glm::uvec3 dispatchSize = (ddgiProbeDimentions + GROUP_SIZE - 1u) / GROUP_SIZE;
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
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::DDGI_ProbeIrradiance), VKW::FORMAT_R16G16B16A16_FLOAT, irradianceDims.x, irradianceDims.y, VKW::RESOURCE_ACCESS_SHADER_WRITE, GraphResourceFlags::TEMPORAL | GraphResourceFlags::INIT_CLEAR);
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeData), ddgi.GetProbeDataBufferSize(), VKW::RESOURCE_ACCESS_SHADER_READ);
}

void DDGIProbeLightingPass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(DDGIProbeLighting);

    DDGI& ddgi = g_GraphicsManager->GetDDGI();

    StorageBuffer* ddgiProbeSampleBuffer = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeSampleBuffer));
    StorageBuffer* probeData  = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeData));
    Texture* atlasIrradiance  = graph.GetTemporalTextureCurrent(RESOURCE_ID(TextureID::DDGI_ProbeIrradiance));
    Texture* atlasIrradianceHistory = graph.GetTemporalTextureHistory(RESOURCE_ID(TextureID::DDGI_ProbeIrradiance));

    auto& dependencyManager = g_GraphicsManager->GetDependencyManager();

    ///////////////////
    // temporary clear, because we don't have a proper reprojection
    //dependencyManager.ResourceBarrier(context, atlasIrradiance->GetResource(), VKW::RESOURCE_ACCESS_TRANSFER_DST, VKW::STAGE_TRANSFER);
    //float clearColor[4] = { 0.f, 0.f, 0.f, 0.f };
    //context.CmdClearColorImage(atlasIrradiance->GetResource(), clearColor);
    // remove later
    ///////////////////


    dependencyManager.ResourceBarrier(context, ddgiProbeSampleBuffer->GetResource(),    VKW::RESOURCE_ACCESS_SHADER_READ,   VKW::STAGE_COMPUTE);
    dependencyManager.ResourceBarrier(context, atlasIrradiance->GetResource(),          VKW::RESOURCE_ACCESS_SHADER_WRITE,  VKW::STAGE_COMPUTE);
    dependencyManager.ResourceBarrier(context, atlasIrradianceHistory->GetResource(),   VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);
    dependencyManager.ResourceBarrier(context, probeData->GetResource(),                VKW::RESOURCE_ACCESS_SHADER_READ,   VKW::STAGE_COMPUTE);

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
    binder.FlushDescriptorWrites();

    context.CmdBindComputePipeline(entry->GetPipeline());
    context.CmdBindComputeDescriptorSets(entry->GetLayout(), binder.GetTargetSetID(), 1, &binder.GetDescriptorSet());

    glm::uvec2 const atlasDims = ddgi.GetProbeAtlasIrradianceDimentions();
    glm::uvec2 const groupSize{ 8, 8 };
    glm::uvec2 const dispatchSize = (atlasDims + groupSize - 1u) / groupSize;
    context.CmdDispatch(dispatchSize.x, dispatchSize.y, 1);
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
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::DDGI_AtlasVisibility), VKW::FORMAT_R16G16_FLOAT, visibilityDims.x, visibilityDims.y, VKW::RESOURCE_ACCESS_SHADER_WRITE, GraphResourceFlags::INIT_CLEAR | GraphResourceFlags::TEMPORAL);
}

void DDGIProbeVisibilityPass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(DDGIProbeVisibility);

    DDGI& ddgi = g_GraphicsManager->GetDDGI();

    StorageBuffer* ddgiProbeSampleBuffer = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeSampleBuffer));
    StorageBuffer* probeData  = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeData));
    Texture* atlasVisibility  = graph.GetTemporalTextureCurrent(RESOURCE_ID(TextureID::DDGI_AtlasVisibility));
    Texture* atlasVisibilityHistory = graph.GetTemporalTextureHistory(RESOURCE_ID(TextureID::DDGI_AtlasVisibility));

    DependencyManager& dependencyManager = g_GraphicsManager->GetDependencyManager();

    dependencyManager.ResourceBarrier(context, ddgiProbeSampleBuffer->GetResource(),    VKW::RESOURCE_ACCESS_SHADER_READ,   VKW::STAGE_COMPUTE);
    dependencyManager.ResourceBarrier(context, probeData->GetResource(),                VKW::RESOURCE_ACCESS_SHADER_READ,   VKW::STAGE_COMPUTE);
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
    binder.FlushDescriptorWrites();

    context.CmdBindComputePipeline(entry->GetPipeline());
    context.CmdBindComputeDescriptorSets(entry->GetLayout(), binder.GetTargetSetID(), 1, &binder.GetDescriptorSet());

    glm::uvec2 const atlasDims = ddgi.GetProbeAtlasVisibilityDimentions();
    glm::uvec2 const groupSize{ 8, 8 };
    glm::uvec2 const dispatchSize = (atlasDims + groupSize - 1u) / groupSize;
    context.CmdDispatch(dispatchSize.x, dispatchSize.y, 1);

}



////////////////////////////////////////////////
PassID DDGIProbeBlendPass::GetID() const
{
    return PassID::DDGIProbeBlend;
}

void DDGIProbeBlendPass::RegisterResources(RenderGraph& graph)
{
}

void DDGIProbeBlendPass::Render(RenderGraph& graph, VKW::Context& context)
{
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
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeData),                 g_GraphicsManager->GetDDGI().GetProbeDataBufferSize(), VKW::RESOURCE_ACCESS_SHADER_READ);

    glm::uvec2 ddgiAtlasDims = g_GraphicsManager->GetDDGI().GetProbeAtlasIrradianceDimentions();
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::DDGI_ProbeIrradiance), VKW::FORMAT_R16G16B16A16_FLOAT, ddgiAtlasDims.x, ddgiAtlasDims.y, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, GraphResourceFlags::TEMPORAL | GraphResourceFlags::INIT_CLEAR);

    DRE::U32 renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth;
    DRE::U32 renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

    auto gBufferFormats = g_GraphicsManager->GetGBufferFormats();

    graph.RegisterRenderTarget(this,
        RESOURCE_ID(TextureID::DisplayEncodedImage),
        gBufferFormats[0], renderWidth, renderHeight,
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

        glm::uvec3 const probeDebugGroupSize{ 4, 4, 4 };
        glm::uvec3 const probeDebugDispatchSize = GetComputeGroupCount(g_GraphicsManager->GetDDGI().GetProbeCount3D(), probeDebugGroupSize);
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
        drawBinder.AddSampledTexture(3, ddgiProbeIrradiance);
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
