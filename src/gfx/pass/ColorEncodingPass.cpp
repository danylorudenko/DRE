#include <gfx\pass\ColorEncodingPass.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>
#include <gfx\renderer\DrawBatcher.hpp>

#include <engine\scene\Scene.hpp>

namespace GFX
{

PassID ColorEncodingPass::GetID() const
{
    return PassID::ColorEncoding;
}

void ColorEncodingPass::Initialize(RenderGraph& graph)
{
}

void ColorEncodingPass::RegisterResources(RenderGraph& graph)
{
    graph.RegisterStorageTexture(this, TextureID::ForwardRT, VKW::FORMAT_B8G8R8A8_UNORM, g_GraphicsManager->GetRenderingWidth(), g_GraphicsManager->GetRenderingHeight(),
        VKW::RESOURCE_ACCESS_SHADER_READ, VKW::STAGE_COMPUTE, 0);

    graph.RegisterStorageTexture(this, TextureID::DisplayEncodedImage, 
        VKW::FORMAT_B8G8R8A8_UNORM, g_GraphicsManager->GetRenderingWidth(), g_GraphicsManager->GetRenderingHeight(), VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE, 1);

    graph.RegisterUniformBuffer(this, VKW::STAGE_COMPUTE, 2);
}

void ColorEncodingPass::Render(RenderGraph& graph, VKW::Context& context)
{
    VKW::ImageResourceView* forwardRT = graph.GetTexture(TextureID::ForwardRT)->GetShaderView();
    VKW::ImageResourceView* encodedImage = graph.GetTexture(TextureID::DisplayEncodedImage)->GetShaderView();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, forwardRT->parentResource_, VKW::RESOURCE_ACCESS_SHADER_READ, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, encodedImage->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);

    VKW::PipelineLayout* layout = graph.GetPassPipelineLayout(GetID());
    VKW::Pipeline* pipeline = g_GraphicsManager->GetPipelineDB().GetPipeline("color_encode");
    VKW::DescriptorSet set = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());

    std::uint32_t const firstSet = g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->GetGlobalSetLayoutsCount();

    context.CmdBindComputeDescriptorSets(layout, firstSet, 1, &set);
    context.CmdBindComputePipeline(pipeline);

    glm::uvec2 rtSize{ g_GraphicsManager->GetRenderingWidth(), g_GraphicsManager->GetRenderingHeight() };
    glm::uvec2 const groupSize{ 8, 8 };
    glm::uvec2 const dispatchSize = rtSize / groupSize + glm::uvec2{ 1, 1 };
    context.CmdDispatch(dispatchSize[0], dispatchSize[1], 1);
}

}
