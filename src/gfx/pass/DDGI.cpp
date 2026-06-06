#include <gfx\pass\DDGI.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>

#include <engine\data\GeometryLibrary.hpp>

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
    glm::uvec3 const ddgiProbeDimentions = GetProbeCount3D();
    return ddgiProbeDimentions.x * ddgiProbeDimentions.y * ddgiProbeDimentions.z;
}

DRE::U32 DDGI::GetProbeDataBufferSize() const
{
    return sizeof(DDGIProbeData) * GetProbeTotalCount();
}

DDGIConstantBuffer DDGI::GetConstantBuffer() const
{
    glm::uvec3 const ddgiProbeDimentions = GetProbeCount3D();
    glm::vec3 const ddgiProbeWorldDistance = glm::vec3(2.0f); // TODO: make this a setting

    DDGIConstantBuffer cb{};
    cb.probesDimentions = ddgiProbeDimentions;
    cb.probesWorldDistance = ddgiProbeWorldDistance;
    cb.probeDebugScale = 0.25f; // TODO: make this a setting

    cb.probeGeometryVertexCount = m_ProbeDebugSphereGPU->GetVertexCount();
    cb.probeGeometryIndexCount = m_ProbeDebugSphereGPU->GetIndexCount();
    return cb;
}


PassID DDGIProbeTracePass::GetID() const
{
    return PassID::DDGIProbeTrace;
}

void DDGIProbeTracePass::RegisterResources(RenderGraph& graph)
{
}

void DDGIProbeTracePass::Initialize(RenderGraph& graph)
{
}

void DDGIProbeTracePass::Render(RenderGraph& graph, VKW::Context& context)
{
}


PassID DDGIProbeBlendPass::GetID() const
{
    return PassID::DDGIProbeBlend;
}

void DDGIProbeBlendPass::RegisterResources(RenderGraph& graph)
{
}

void DDGIProbeBlendPass::Initialize(RenderGraph& graph)
{
}

void DDGIProbeBlendPass::Render(RenderGraph& graph, VKW::Context& context)
{
}


PassID DDGIProbeLightingPass::GetID() const
{
    return PassID::DDGIProbeLighting;
}

void DDGIProbeLightingPass::RegisterResources(RenderGraph& graph)
{
}

void DDGIProbeLightingPass::Initialize(RenderGraph& graph)
{
}

void DDGIProbeLightingPass::Render(RenderGraph& graph, VKW::Context& context)
{
}


PassID DDGIProbeScatterPass::GetID() const
{
    return PassID::DDGIProbeScatter;
}

void DDGIProbeScatterPass::RegisterResources(RenderGraph& graph)
{
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeData), g_GraphicsManager->GetDDGI().GetProbeDataBufferSize(), VKW::RESOURCE_ACCESS_GENERIC_WRITE);

}

void DDGIProbeScatterPass::Initialize(RenderGraph& graph)
{
}

void DDGIProbeScatterPass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(DDGIProbeScatter);

    glm::uvec3 GROUP_SIZE{ 4, 4, 4, };

    StorageBuffer* ddgiProbeData = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeData));
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeData->GetResource(), VKW::RESOURCE_ACCESS_GENERIC_WRITE, VKW::STAGE_COMPUTE);

    PipelineEntry* entry = g_GraphicsManager->GetPipelineDB().GetEntry("ddgi_probe_scatter");
    UniformProxy uniform = graph.AllocateUniform(GetID(), context, sizeof(DDGIConstantBuffer));

    uniform.WriteMember140(g_GraphicsManager->GetDDGI().GetConstantBuffer());
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
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeData),                 g_GraphicsManager->GetDDGI().GetProbeDataBufferSize(), VKW::RESOURCE_ACCESS_GENERIC_READ);

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


        ddgiUniform.WriteMember140(g_GraphicsManager->GetDDGI().GetConstantBuffer());
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

        g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, output->GetResource(), VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
        g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, depthBuffer->GetResource(), VKW::RESOURCE_ACCESS_DEPTH_STENCIL_ATTACHMENT, VKW::STAGE_ALL_GRAPHICS);
        g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeData->GetResource(), VKW::RESOURCE_ACCESS_GENERIC_READ, VKW::STAGE_ALL_GRAPHICS);
        g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeIndirectArgs->GetResource(), VKW::RESOURCE_ACCESS_INDIRECT_ARGS, VKW::STAGE_ALL_GRAPHICS);

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