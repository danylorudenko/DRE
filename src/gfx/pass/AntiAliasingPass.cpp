#include <gfx\pass\AntiAliasingPass.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>
#include <gfx\renderer\DrawBatcher.hpp>

#include <engine\scene\Scene.hpp>

namespace GFX
{

PassID AntiAliasingPass::GetID() const
{
    return PassID::AntiAliasing;
}

void AntiAliasingPass::Initialize(RenderGraph& graph)
{
}

void AntiAliasingPass::RegisterResources(RenderGraph& graph)
{
    graph.RegisterUniformBuffer(this, VKW::STAGE_COMPUTE, 0);

    std::uint32_t renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth, renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

    graph.RegisterTexture(this, 
        RESOURCE_ID(TextureID::Velocity),
        VKW::FORMAT_R16G16_FLOAT, renderWidth, renderHeight, 
        VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE, 1);

    graph.RegisterTexture(this,
        RESOURCE_ID(TextureID::WaterColor),
        g_GraphicsManager->GetMainColorFormat(), renderWidth, renderHeight,
        VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE, 2);

    VKW::ResourceAccess historyAccess = VKW::ResourceAccess(VKW::RESOURCE_ACCESS_SHADER_SAMPLE | std::uint64_t(VKW::RESOURCE_ACCESS_SHADER_WRITE));
    graph.RegisterStandaloneTexture(RESOURCE_ID(TextureID::ColorHistoryBuffer0), VKW::FORMAT_B8G8R8A8_UNORM, renderWidth, renderHeight, historyAccess);
    graph.RegisterStandaloneTexture(RESOURCE_ID(TextureID::ColorHistoryBuffer1), VKW::FORMAT_B8G8R8A8_UNORM, renderWidth, renderHeight, historyAccess);

    graph.RegisterTextureSlot(this, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE, 3);
    graph.RegisterTextureSlot(this, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE, 4);

    graph.RegisterTexture(this,
        RESOURCE_ID(TextureID::MainDepth), g_GraphicsManager->GetMainDepthFormat(), renderWidth, renderHeight,
        VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE, 5);
}


void AntiAliasingPass::Render(RenderGraph& graph, VKW::Context& context)
{
    Texture* historyBuffers[2] = { graph.GetTexture(RESOURCE_ID(TextureID::ColorHistoryBuffer0)), graph.GetTexture(RESOURCE_ID(TextureID::ColorHistoryBuffer1)) };

    VKW::ImageResourceView* colorInput = graph.GetTexture(RESOURCE_ID(TextureID::WaterColor))->GetShaderView();
    VKW::ImageResourceView* velocity = graph.GetTexture(RESOURCE_ID(TextureID::Velocity))->GetShaderView();
    VKW::ImageResourceView* history = historyBuffers[g_GraphicsManager->GetPrevFrameID()]->GetShaderView();
    VKW::ImageResourceView* taaOutput = historyBuffers[g_GraphicsManager->GetCurrentFrameID()]->GetShaderView();
    VKW::ImageResourceView* mainDepth = graph.GetTexture(RESOURCE_ID(TextureID::MainDepth))->GetShaderView();

    VKW::DescriptorSet passSet = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());
    VKW::DescriptorManager::WriteDesc writeDesc;
    writeDesc.AddSampledImage(history, 3);      // input
    writeDesc.AddStorageImage(taaOutput, 4);    // output
    g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->WriteDescriptorSet(passSet, writeDesc);

    {
        glm::vec4 taaSettings{ g_GraphicsManager->GetGraphicsSettings().m_AlphaTAA, g_GraphicsManager->GetGraphicsSettings().m_VarianceGammaTAA, 0.0f, 0.0f };
        UniformProxy uniform = graph.GetPassUniform(GetID(), context, sizeof(taaSettings));
        uniform.WriteMember140(taaSettings);
    }

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, colorInput->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, velocity->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, history->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, taaOutput->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, mainDepth->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);

    VKW::PipelineLayout* layout = graph.GetPassPipelineLayout(GetID());
    std::uint32_t const firstSet = g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->GetGlobalSetLayoutsCount();
    context.CmdBindComputeDescriptorSets(layout, firstSet, 1, &passSet);

    VKW::Pipeline* pipeline = g_GraphicsManager->GetPipelineDB().GetPipeline("temporal_AA");
    context.CmdBindComputePipeline(pipeline);

    glm::uvec2 rtSize{ g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth, g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight };
    glm::uvec2 const groupSize{ 8, 8 };
    glm::uvec2 const dispatchSize = rtSize / groupSize + glm::uvec2{ 1, 1 };
    context.CmdDispatch(dispatchSize.x, dispatchSize.y, 1);
}

}
