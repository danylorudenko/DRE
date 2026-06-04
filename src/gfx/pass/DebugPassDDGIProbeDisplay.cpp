#include <gfx\pass\DebugPassDDGIProbeDisplay.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>
#include <gfx\renderer\DDGI.hpp>

#include <engine\data\GeometryLibrary.hpp>

namespace GFX
{

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

    std::uint32_t renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth,
        renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

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

    StorageBuffer* ddgiProbeIndirectArgs = graph.GetBuffer(RESOURCE_ID(BufferID::DebugPassDDGIProbeIndirectArgs));
    StorageBuffer* ddgiProbeData         = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeData));

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeIndirectArgs->GetResource(), VKW::RESOURCE_ACCESS_GENERIC_WRITE, VKW::STAGE_TRANSFER);
    context.CmdFillBuffer(ddgiProbeIndirectArgs->GetResource(), 0, sizeof(DrawIndexedIndirectCommand), 0);

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeIndirectArgs->GetResource(), VKW::RESOURCE_ACCESS_GENERIC_RW,   VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeData->GetResource(),         VKW::RESOURCE_ACCESS_GENERIC_READ, VKW::STAGE_COMPUTE);


    UniformProxy ddgiUniform = graph.AllocateUniform(GetID(), context, sizeof(DDGIConstantBuffer));
    ddgiUniform.WriteMember140(DDGI::GetConstantBuffer());
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



    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, graph.GetTexture(RESOURCE_ID(TextureID::GBufferA_DiffuseRoughness))->GetResource(), VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, graph.GetTexture(RESOURCE_ID(TextureID::GBufferB_NormalMetalness))->GetResource(), VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, graph.GetTexture(RESOURCE_ID(TextureID::GBufferC_Velocity))->GetResource(), VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, graph.GetTexture(RESOURCE_ID(TextureID::GBufferD_ObjectIDBuffer))->GetResource(), VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeData->GetResource(), VKW::RESOURCE_ACCESS_GENERIC_READ, VKW::STAGE_ALL_GRAPHICS);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeIndirectArgs->GetResource(), VKW::RESOURCE_ACCESS_INDIRECT_ARGS, VKW::STAGE_ALL_GRAPHICS);

    PipelineEntry* drawSpheresEntry = g_GraphicsManager->GetPipelineDB().GetEntry("debug_view_ddgi_probes_draw");
    ResourceBinder drawBinder = g_GraphicsManager->CreateResourceBinder(drawSpheresEntry, 0);
    drawBinder.AddStorageBuffer(0, ddgiProbeIndirectArgs);
    drawBinder.AddStorageBuffer(1, ddgiProbeData);
    drawBinder.AddUniform(2, &ddgiUniform);
    drawBinder.FlushDescriptorWrites();

    context.CmdBindGraphicsDescriptorSets(drawSpheresEntry->GetLayout(), drawBinder.GetTargetSetID(), 1, &drawBinder.GetDescriptorSet());
    context.CmdDrawIndexedIndirect(ddgiProbeIndirectArgs->GetResource());

}

}
