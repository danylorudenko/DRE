#include <gfx\pass\LightingPass.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>

namespace GFX
{

PassID LightingPass::GetID() const
{
    return PassID::Lighting;
}

void LightingPass::RegisterResources(RenderGraph& graph)
{
    std::uint32_t renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth;
    std::uint32_t renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

    auto gBufferFormats = g_GraphicsManager->GetGBufferFormats();

    graph.RegisterTexture(this,
        RESOURCE_ID(TextureID::ForwardColor),
        g_GraphicsManager->GetMainColorFormat(), renderWidth, renderHeight,
        VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE, 0);

    graph.RegisterTexture(this,
        RESOURCE_ID(TextureID::GBufferA),
        gBufferFormats[0], renderWidth, renderHeight,
        VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE, 1);

    graph.RegisterTexture(this,
        RESOURCE_ID(TextureID::GBufferB),
        gBufferFormats[1], renderWidth, renderHeight,
        VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE, 2);

    graph.RegisterTexture(this,
        RESOURCE_ID(TextureID::MainDepth),
        g_GraphicsManager->GetMainDepthFormat(), renderWidth, renderHeight,
        VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE, 3);
}

void LightingPass::Initialize(RenderGraph&)
{
}

void LightingPass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(Lighting);

    VKW::ImageResourceView* forwardColor = graph.GetTexture(RESOURCE_ID(TextureID::ForwardColor))->GetShaderView();
    VKW::ImageResourceView* gbufferA = graph.GetTexture(RESOURCE_ID(TextureID::GBufferA))->GetShaderView();
    VKW::ImageResourceView* gbufferB = graph.GetTexture(RESOURCE_ID(TextureID::GBufferB))->GetShaderView();
    VKW::ImageResourceView* depth = graph.GetTexture(RESOURCE_ID(TextureID::MainDepth))->GetShaderView();

    auto& dependencyManager = g_GraphicsManager->GetDependencyManager();
    dependencyManager.ResourceBarrier(context, forwardColor->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);
    dependencyManager.ResourceBarrier(context, gbufferA->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);
    dependencyManager.ResourceBarrier(context, gbufferB->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);
    dependencyManager.ResourceBarrier(context, depth->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);

    VKW::PipelineLayout* layout = graph.GetPassPipelineLayout(GetID());
    VKW::Pipeline* pipeline = g_GraphicsManager->GetPipelineDB().GetPipeline("lighting_deferred");
    VKW::DescriptorSet set = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());

    context.CmdBindComputeDescriptorSets(layout, graph.GetPassSetBinding(), 1, &set);
    context.CmdBindComputePipeline(pipeline);

    std::uint32_t renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth;
    std::uint32_t renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

    constexpr std::uint32_t groupSizeX = 8;
    constexpr std::uint32_t groupSizeY = 8;
    std::uint32_t dispatchX = (renderWidth + groupSizeX - 1) / groupSizeX;
    std::uint32_t dispatchY = (renderHeight + groupSizeY - 1) / groupSizeY;

    context.CmdDispatch(dispatchX, dispatchY, 1);
}

}
