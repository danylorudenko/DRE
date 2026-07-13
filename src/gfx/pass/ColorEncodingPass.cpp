#include <gfx\pass\ColorEncodingPass.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>

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

    graph.RegisterTexture(this, RESOURCE_ID(TextureID::ColorHistoryBuffer), VKW::FORMAT_B8G8R8A8_UNORM, renderWidth, renderHeight, VKW::RESOURCE_ACCESS_SHADER_READ, GraphResourceFlags::TEMPORAL);

    graph.RegisterTexture(this, RESOURCE_ID(TextureID::DisplayEncodedImage),
        g_GraphicsManager->GetFinalImageFormat(), renderWidth, renderHeight, VKW::RESOURCE_ACCESS_SHADER_WRITE);
}

void ColorEncodingPass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(ColorEncoding);

    Texture* taaOutput = graph.GetTemporalTextureCurrent(RESOURCE_ID(TextureID::ColorHistoryBuffer));
    Texture* encodedImage = graph.GetTexture(RESOURCE_ID(TextureID::DisplayEncodedImage));

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, taaOutput->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_READ, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, encodedImage->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);

    UniformProxy uniform = graph.AllocateUniform(GetID(), context, sizeof(glm::vec4));
    float const useACES = g_GraphicsManager->GetGraphicsSettings().m_UseACESEncoding ? 1.0f : 0.0f;
    float const exposure = glm::exp2(-g_GraphicsManager->GetGraphicsSettings().m_ExposureEV);
    uniform.WriteMember140(glm::vec4{ useACES, exposure, 0.0f, 0.0f });
    uniform.FlushWrites();

    PipelineEntry* pipelineEntry = g_GraphicsManager->GetPipelineDB().GetEntry("color_encode");

    ResourceBinder binder = g_GraphicsManager->CreateResourceBinder(pipelineEntry, 0);
    binder.AddStorageTexture(0, taaOutput);
    binder.AddStorageTexture(1, encodedImage);
    binder.AddUniform(2, &uniform);
    binder.FlushDescriptorWrites();

    context.CmdBindComputeDescriptorSets(pipelineEntry->GetLayout(), binder.GetTargetSetID(), 1, &binder.GetDescriptorSet());
    context.CmdBindComputePipeline(pipelineEntry->GetPipeline());

    glm::uvec2 rtSize{ g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth, g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight };
    glm::uvec2 const groupSize{ 8, 8 };
    glm::uvec2 const dispatchSize = rtSize / groupSize + glm::uvec2{ 1, 1 };
    context.CmdDispatch(dispatchSize.x, dispatchSize.y, 1);
}

}
