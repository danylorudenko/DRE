#include <gfx\pass\DebugPass.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>
#include <gfx\renderer\DrawBatcher.hpp>

#include <engine\scene\Scene.hpp>

namespace GFX
{

PassID DebugPass::GetID() const
{
    return PassID::Debug;
}

void DebugPass::Initialize(RenderGraph& graph)
{
}

void DebugPass::RegisterResources(RenderGraph& graph)
{
    std::uint32_t renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth, renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

    graph.RegisterTexture(this, TextureID::DisplayEncodedImage, VKW::FORMAT_B8G8R8A8_UNORM,
        renderWidth, renderHeight,
        VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE,
        0);

    graph.RegisterTexture(this, TextureID::FFTHxt, VKW::FORMAT_R32G32B32A32_FLOAT, WATER_DIM, WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE, 1);
}

void DebugPass::Render(RenderGraph& graph, VKW::Context& context)
{
    VKW::ImageResourceView* output = graph.GetTexture(TextureID::DisplayEncodedImage)->GetShaderView();
    VKW::ImageResourceView* Hxt = graph.GetTexture(TextureID::FFTHxt)->GetShaderView();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, output->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, Hxt->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);

    VKW::DescriptorSet set = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());

    VKW::PipelineLayout* layout = graph.GetPassPipelineLayout(GetID());
    VKW::Pipeline* pipeline = g_GraphicsManager->GetPipelineDB().GetPipeline("debug_view");
    std::uint32_t const firstSet = g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->GetGlobalSetLayoutsCount();
    context.CmdBindComputeDescriptorSets(layout, firstSet, 1, &set);
    context.CmdBindComputePipeline(pipeline);

    glm::uvec2 imageSize{ WATER_DIM, WATER_DIM };
    glm::uvec2 const groupSize{ 8, 8 };
    glm::uvec2 const dispatchSize = imageSize / groupSize;
    context.CmdDispatch(dispatchSize.x, dispatchSize.y, 1);
}

}
