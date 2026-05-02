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
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DebugPassDDGIProbeIndirectArgs), sizeof(DrawIndexedIndirectCommand), VKW::RESOURCE_ACCESS_INDIRECT_ARGS, VKW::STAGE_ALL_GLOBAL, 0);
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeData),                 DDGI::GetProbeDataBufferSize(), VKW::RESOURCE_ACCESS_GENERIC_READ,  VKW::STAGE_ALL_GLOBAL, 1);

    graph.RegisterRenderTarget(this, RESOURCE_ID(TextureID::DisplayEncodedImage), 
        g_GraphicsManager->GetFinalImageFormat(),
        g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth, g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight, 0);
}

void DebugPassDDGIProbeDisplay::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(DebugPassDDGIProbeDisplay);

    VKW::BufferResource* ddgiProbeIndirectArgs = graph.GetBuffer(RESOURCE_ID(BufferID::DebugPassDDGIProbeIndirectArgs))->GetResource();
    VKW::BufferResource* ddgiProbeData         = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeData))->GetResource();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeIndirectArgs, VKW::RESOURCE_ACCESS_GENERIC_WRITE, VKW::STAGE_TRANSFER);
    context.CmdFillBuffer(ddgiProbeIndirectArgs, 0, sizeof(DrawIndexedIndirectCommand), 0);

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeIndirectArgs, VKW::RESOURCE_ACCESS_GENERIC_RW,   VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeData,         VKW::RESOURCE_ACCESS_GENERIC_READ, VKW::STAGE_COMPUTE);

    glm::uvec3 const probeDebugGroupSize{ 4, 4, 4 };
    glm::uvec3 const probeDebugDispatchSize = GetComputeGroupCount(DDGI::GetProbeCount3D(), probeDebugGroupSize);
    // find pipeline here!
    context.CmdDispatch(probeDebugDispatchSize.x, probeDebugDispatchSize.y, probeDebugDispatchSize.z);


    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, graph.GetTexture(RESOURCE_ID(TextureID::DisplayEncodedImage))->GetResource(), VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeData, VKW::RESOURCE_ACCESS_GENERIC_READ, VKW::STAGE_ALL_GRAPHICS);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeIndirectArgs, VKW::RESOURCE_ACCESS_INDIRECT_ARGS, VKW::STAGE_ALL_GRAPHICS);

    VKW::Pipeline* pipeline = g_GraphicsManager->GetPipelineDB().GetPipeline("debug_view_ddgi_probes");
    VKW::DescriptorSet set = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());
    context.CmdBindGraphicsDescriptorSets(graph.GetPassPipelineLayout(GetID()), graph.GetPassSetBinding(), 1, &set);

}

}
