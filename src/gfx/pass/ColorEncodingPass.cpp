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
    std::uint32_t renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth, renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

    graph.RegisterTextureSlot(this, VKW::RESOURCE_ACCESS_SHADER_READ, VKW::STAGE_COMPUTE, 0);
    graph.RegisterStandaloneTexture(RESOURCE_ID(TextureID::ColorHistoryBuffer0), VKW::FORMAT_B8G8R8A8_UNORM, renderWidth, renderHeight, VKW::RESOURCE_ACCESS_SHADER_READ);
    graph.RegisterStandaloneTexture(RESOURCE_ID(TextureID::ColorHistoryBuffer1), VKW::FORMAT_B8G8R8A8_UNORM, renderWidth, renderHeight, VKW::RESOURCE_ACCESS_SHADER_READ);

    graph.RegisterTexture(this, RESOURCE_ID(TextureID::DisplayEncodedImage),
        VKW::FORMAT_B8G8R8A8_UNORM, renderWidth, renderHeight, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE, 1);

    graph.RegisterUniformBuffer(this, VKW::STAGE_COMPUTE, 2);
}

void ColorEncodingPass::Render(RenderGraph& graph, VKW::Context& context)
{
    Texture* historyBuffers[2] = { 
        graph.GetTexture(RESOURCE_ID(TextureID::ColorHistoryBuffer0)),
        graph.GetTexture(RESOURCE_ID(TextureID::ColorHistoryBuffer1))
    };

    VKW::ImageResourceView* taaOutput = historyBuffers[g_GraphicsManager->GetCurrentFrameID()]->GetShaderView();
    VKW::ImageResourceView* encodedImage = graph.GetTexture(RESOURCE_ID(TextureID::DisplayEncodedImage))->GetShaderView();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, taaOutput->parentResource_, VKW::RESOURCE_ACCESS_SHADER_READ, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, encodedImage->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);

    UniformProxy uniform = graph.GetPassUniform(GetID(), context, sizeof(glm::vec4));
    float const useACES = g_GraphicsManager->GetGraphicsSettings().m_UseACESEncoding ? 1.0f : 0.0f;
    float const exposure = glm::exp2(-g_GraphicsManager->GetGraphicsSettings().m_ExposureEV);
    uniform.WriteMember140(glm::vec4{ useACES, exposure, 0.0f, 0.0f });

    VKW::DescriptorSet set = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());

    VKW::DescriptorManager::WriteDesc writeDesc;
    writeDesc.AddStorageImage(taaOutput, 0);
    g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->WriteDescriptorSet(set,writeDesc);

    VKW::PipelineLayout* layout = graph.GetPassPipelineLayout(GetID());
    VKW::Pipeline* pipeline = g_GraphicsManager->GetPipelineDB().GetPipeline("color_encode");
    std::uint32_t const firstSet = g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->GetGlobalSetLayoutsCount();
    context.CmdBindComputeDescriptorSets(layout, firstSet, 1, &set);
    context.CmdBindComputePipeline(pipeline);

    glm::uvec2 rtSize{ g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth, g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight };
    glm::uvec2 const groupSize{ 8, 8 };
    glm::uvec2 const dispatchSize = rtSize / groupSize + glm::uvec2{ 1, 1 };
    context.CmdDispatch(dispatchSize.x, dispatchSize.y, 1);
}

}
