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

}