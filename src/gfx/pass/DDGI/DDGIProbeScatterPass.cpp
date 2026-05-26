#include <gfx\pass\DDGI\DDGIProbeScatterPass.hpp>

#include <gfx\renderer\DDGI.hpp>
#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>


#include <common\global_illumination\ddgi_common.slang>

namespace GFX
{

PassID DDGIProbeScatterPass::GetID() const
{
    return PassID::DDGIProbeScatter;
}

void DDGIProbeScatterPass::RegisterResources(RenderGraph& graph)
{
    graph.RegisterUniformBuffer(this, VKW::STAGE_COMPUTE, 0);
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeData), GFX::DDGI::GetProbeDataBufferSize(), VKW::RESOURCE_ACCESS_GENERIC_WRITE, VKW::STAGE_COMPUTE, 1);

}

void DDGIProbeScatterPass::Initialize(RenderGraph& graph)
{
}

void DDGIProbeScatterPass::Render(RenderGraph& graph, VKW::Context& context)
{
    glm::uvec3 GROUP_SIZE{ 4, 4, 4, };

    GFX::StorageBuffer* ddgiProbeData = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeData));
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeData->GetResource(), VKW::RESOURCE_ACCESS_GENERIC_WRITE, VKW::STAGE_COMPUTE);

    context.CmdBindComputePipeline(g_GraphicsManager->GetPipelineDB().GetEntry("ddgi_probe_scatter")->GetPipeline());

    VKW::DescriptorSet passSet = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());
    context.CmdBindComputeDescriptorSets(graph.GetPassPipelineLayout(GetID()), graph.GetPassSetBinding(), 1, &passSet);

    graph.GetPassUniform(GetID(), context, sizeof(DDGIConstantBuffer));

    auto& settings = g_GraphicsManager->GetGraphicsSettings();
    glm::uvec3 const ddgiProbeDimentions = glm::uvec3(settings.m_DDGIProbeCountX, settings.m_DDGIProbeCountY, settings.m_DDGIProbeCountZ);

    glm::uvec3 dispatchSize = (ddgiProbeDimentions + GROUP_SIZE - 1u) / GROUP_SIZE;
    context.CmdDispatch(dispatchSize.x, dispatchSize.y, dispatchSize.z);
}

}