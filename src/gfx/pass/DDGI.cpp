#include <gfx\pass\DDGI.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>

#include <engine\data\GeometryLibrary.hpp>

#include <common\global_illumination\ddgi_common.slang>

namespace GFX::DDGI
{

glm::uvec3 GetProbeCount3D()
{
    auto const& settings = g_GraphicsManager->GetGraphicsSettings();
    return glm::uvec3(settings.m_DDGIProbeCountX, settings.m_DDGIProbeCountY, settings.m_DDGIProbeCountZ);
}

DRE::U32 GetProbeTotalCount()
{
    glm::uvec3 const ddgiProbeDimentions = GetProbeCount3D();
    return ddgiProbeDimentions.x * ddgiProbeDimentions.y * ddgiProbeDimentions.z;
}

DRE::U32 GetProbeDataBufferSize()
{
    return sizeof(DDGIProbeData) * GetProbeTotalCount();
}

DDGIConstantBuffer GetConstantBuffer(DRE::U32 probeSphereVertexCount, DRE::U32 probeSphereIndexCount)
{
    auto const& settings = g_GraphicsManager->GetGraphicsSettings();

    glm::uvec3 const ddgiProbeDimentions = GetProbeCount3D();
    glm::vec3 const ddgiProbeWorldDistance = glm::vec3(1.0f); // TODO: make this a setting

    DDGIConstantBuffer cb{};
    cb.probesDimentions = ddgiProbeDimentions;
    cb.probesWorldDistance = ddgiProbeWorldDistance;

    cb.probeGeometryVertexCount = probeSphereVertexCount;
    cb.probeGeometryIndexCount = probeSphereIndexCount;
    return cb;
}

}

namespace GFX
{

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
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeData), GFX::DDGI::GetProbeDataBufferSize(), VKW::RESOURCE_ACCESS_GENERIC_WRITE);

}

void DDGIProbeScatterPass::Initialize(RenderGraph& graph)
{
}

void DDGIProbeScatterPass::Render(RenderGraph& graph, VKW::Context& context)
{
    glm::uvec3 GROUP_SIZE{ 4, 4, 4, };

    StorageBuffer* ddgiProbeData = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeData));
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeData->GetResource(), VKW::RESOURCE_ACCESS_GENERIC_WRITE, VKW::STAGE_COMPUTE);

    PipelineEntry* entry = g_GraphicsManager->GetPipelineDB().GetEntry("ddgi_probe_scatter");
    UniformProxy uniform = graph.AllocateUniform(GetID(), context, sizeof(DDGIConstantBuffer));

    uniform.WriteMember140(GFX::DDGI::GetConstantBuffer());
    uniform.FlushWrites();

    context.CmdBindComputePipeline(entry->GetPipeline());
    ResourceBinder binder = g_GraphicsManager->CreateResourceBinder(entry, 0);
    binder.AddUniform(0, &uniform);
    binder.AddStorageBuffer(1, ddgiProbeData);
    binder.FlushDescriptorWrites();

    context.CmdBindComputeDescriptorSets(entry->GetLayout(), binder.GetTargetSetID(), 1, &binder.GetDescriptorSet());

    auto& settings = g_GraphicsManager->GetGraphicsSettings();
    glm::uvec3 const ddgiProbeDimentions = glm::uvec3(settings.m_DDGIProbeCountX, settings.m_DDGIProbeCountY, settings.m_DDGIProbeCountZ);

    glm::uvec3 dispatchSize = (ddgiProbeDimentions + GROUP_SIZE - 1u) / GROUP_SIZE;
    context.CmdDispatch(dispatchSize.x, dispatchSize.y, dispatchSize.z);
}


DebugPassDDGIProbeDisplay::DebugPassDDGIProbeDisplay(Data::GeometryLibrary* geometryLibrary)
    : m_GeometryLibrary{ geometryLibrary }
    , m_ProbeDebugSphereGPU{ nullptr }
{
}

PassID DebugPassDDGIProbeDisplay::GetID() const
{
    return PassID::DebugDDGIProbeDisplay;
}

void DebugPassDDGIProbeDisplay::Initialize(RenderGraph& graph)
{
    Data::Geometry* ddgiProbeGeometry = m_GeometryLibrary->GetGeometry(DDGI::GetProbeDebugSphereGeometryName());
    m_ProbeDebugSphereGPU = g_GraphicsManager->GetGlobalGeometryManager().FindOrUploadGeometry(ddgiProbeGeometry);
}

void DebugPassDDGIProbeDisplay::RegisterResources(RenderGraph& graph)
{
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DebugPassDDGIProbeIndirectArgs), sizeof(DrawIndexedIndirectCommand), VKW::RESOURCE_ACCESS_INDIRECT_ARGS);
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeData),                 DDGI::GetProbeDataBufferSize(), VKW::RESOURCE_ACCESS_GENERIC_READ);

    DRE::U32 renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth;
    DRE::U32 renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

    auto gBufferFormats = g_GraphicsManager->GetGBufferFormats();

    graph.RegisterRenderTarget(this,
        RESOURCE_ID(TextureID::GBufferA_DiffuseRoughness),
        gBufferFormats[0], renderWidth, renderHeight,
        0);

    graph.RegisterRenderTarget(this,
        RESOURCE_ID(TextureID::GBufferB_NormalMetalness),
        gBufferFormats[1], renderWidth, renderHeight,
        1);

    graph.RegisterRenderTarget(this,
        RESOURCE_ID(TextureID::GBufferC_Velocity),
        gBufferFormats[2], renderWidth, renderHeight,
        2);

    graph.RegisterRenderTarget(this,
        RESOURCE_ID(TextureID::GBufferD_ObjectIDBuffer),
        gBufferFormats[3], renderWidth, renderHeight,
        3);

    graph.RegisterRenderTarget(this,
        RESOURCE_ID(TextureID::DEBUG_TEXTURE),
        VKW::FORMAT_R32G32B32A32_FLOAT, renderWidth, renderHeight,
        4);

    graph.RegisterDepthOnlyTarget(this,
        RESOURCE_ID(TextureID::MainDepth),
        g_GraphicsManager->GetMainDepthFormat(), renderWidth, renderHeight);
}

void DebugPassDDGIProbeDisplay::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(DebugPassDDGIProbeDisplay);

    DRE::U32 renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth;
    DRE::U32 renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

    StorageBuffer* ddgiProbeIndirectArgs = graph.GetBuffer(RESOURCE_ID(BufferID::DebugPassDDGIProbeIndirectArgs));
    StorageBuffer* ddgiProbeData         = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeData));

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeIndirectArgs->GetResource(), VKW::RESOURCE_ACCESS_GENERIC_WRITE, VKW::STAGE_TRANSFER);
    context.CmdFillBuffer(ddgiProbeIndirectArgs->GetResource(), 0, sizeof(DrawIndexedIndirectCommand), 0);

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeIndirectArgs->GetResource(), VKW::RESOURCE_ACCESS_GENERIC_RW,   VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeData->GetResource(),         VKW::RESOURCE_ACCESS_GENERIC_READ, VKW::STAGE_COMPUTE);


    UniformProxy ddgiUniform = graph.AllocateUniform(GetID(), context, sizeof(DDGIConstantBuffer));
    ddgiUniform.WriteMember140(DDGI::GetConstantBuffer(m_ProbeDebugSphereGPU->GetVertexCount(), m_ProbeDebugSphereGPU->GetIndexCount()));
    ddgiUniform.FlushWrites();

    PipelineEntry* indirectFillEntry = g_GraphicsManager->GetPipelineDB().GetEntry("debug_view_ddgi_probes_args");
    ResourceBinder indirectBinder = g_GraphicsManager->CreateResourceBinder(indirectFillEntry, 0);
    indirectBinder.AddStorageBuffer(0, ddgiProbeIndirectArgs);
    indirectBinder.AddStorageBuffer(1, ddgiProbeData);
    indirectBinder.AddUniform(2, &ddgiUniform);
    indirectBinder.FlushDescriptorWrites();

    context.CmdBindComputeDescriptorSets(indirectFillEntry->GetLayout(), indirectBinder.GetTargetSetID(), 1, &indirectBinder.GetDescriptorSet());

    glm::uvec3 const probeDebugGroupSize{ 4, 4, 4 };
    glm::uvec3 const probeDebugDispatchSize = GetComputeGroupCount(DDGI::GetProbeCount3D(), probeDebugGroupSize);
    context.CmdBindPipeline(VKW::BindPoint::Compute, indirectFillEntry->GetPipeline());
    context.CmdDispatch(probeDebugDispatchSize.x, probeDebugDispatchSize.y, probeDebugDispatchSize.z);




    Texture* GBufferA = graph.GetTexture(RESOURCE_ID(TextureID::GBufferA_DiffuseRoughness));
    Texture* GBufferB = graph.GetTexture(RESOURCE_ID(TextureID::GBufferB_NormalMetalness));
    Texture* GBufferC = graph.GetTexture(RESOURCE_ID(TextureID::GBufferC_Velocity));
    Texture* GBufferD = graph.GetTexture(RESOURCE_ID(TextureID::GBufferD_ObjectIDBuffer));
    Texture* DEBUGTEXTURE = graph.GetTexture(RESOURCE_ID(TextureID::DEBUG_TEXTURE));
    Texture* depthBuffer = graph.GetTexture(RESOURCE_ID(TextureID::MainDepth));

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, GBufferA->GetResource(), VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, GBufferB->GetResource(), VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, GBufferC->GetResource(), VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, GBufferD->GetResource(), VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, DEBUGTEXTURE->GetResource(), VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
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

    DRE::U32 constexpr attachmentsCount = 5;
    VKW::ImageResourceView* renderTargets[attachmentsCount] = {
        GBufferA->GetShaderView(),
        GBufferB->GetShaderView(),
        GBufferC->GetShaderView(),
        GBufferD->GetShaderView(),
        DEBUGTEXTURE->GetShaderView()
    };

    context.CmdBeginRendering(attachmentsCount, renderTargets, depthBuffer->GetShaderView(), nullptr);
    context.CmdSetViewport(attachmentsCount, 0, 0, renderWidth, renderHeight);
    context.CmdSetScissor(attachmentsCount, 0, 0, renderWidth, renderHeight);
#ifndef DRE_COMPILE_FOR_RENDERDOC
    context.CmdSetPolygonMode(VKW::POLYGON_FILL);
#endif // DRE_COMPILE_FOR_RENDERDOC

    context.CmdBindVertexBuffer(m_ProbeDebugSphereGPU->GetBuffer(), m_ProbeDebugSphereGPU->GetVertexOffset());
    context.CmdBindIndexBuffer(m_ProbeDebugSphereGPU->GetBuffer(), m_ProbeDebugSphereGPU->GetIndexOffset(), VK_INDEX_TYPE_UINT32);
    context.CmdDrawIndexedIndirect(ddgiProbeIndirectArgs->GetResource());
    context.CmdEndRendering();

}

}