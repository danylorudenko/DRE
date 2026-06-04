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
    std::uint32_t renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth, renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

    graph.RegisterTexture(this, RESOURCE_ID(TextureID::GBufferC_Velocity), g_GraphicsManager->GetVelocityBufferFormat(), renderWidth, renderHeight, VKW::RESOURCE_ACCESS_SHADER_SAMPLE);

    graph.RegisterTexture(this, RESOURCE_ID(TextureID::ForwardColor), g_GraphicsManager->GetMainColorFormat(), renderWidth, renderHeight, VKW::RESOURCE_ACCESS_SHADER_SAMPLE);

    VKW::ResourceAccess historyAccess = VKW::ResourceAccess(VKW::RESOURCE_ACCESS_SHADER_SAMPLE | std::uint64_t(VKW::RESOURCE_ACCESS_SHADER_WRITE));
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::ColorHistoryBuffer0), VKW::FORMAT_B8G8R8A8_UNORM, renderWidth, renderHeight, historyAccess);
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::ColorHistoryBuffer1), VKW::FORMAT_B8G8R8A8_UNORM, renderWidth, renderHeight, historyAccess);

    graph.RegisterTexture(this, RESOURCE_ID(TextureID::MainDepth), g_GraphicsManager->GetMainDepthFormat(), renderWidth, renderHeight, VKW::RESOURCE_ACCESS_SHADER_SAMPLE);
}


void AntiAliasingPass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(AntiAliasing);

    Texture* historyBuffers[2] = { graph.GetTexture(RESOURCE_ID(TextureID::ColorHistoryBuffer0)), graph.GetTexture(RESOURCE_ID(TextureID::ColorHistoryBuffer1)) };

    Texture* colorInput = graph.GetTexture(RESOURCE_ID(TextureID::ForwardColor));
    Texture* velocity = graph.GetTexture(RESOURCE_ID(TextureID::GBufferC_Velocity));
    Texture* history = historyBuffers[g_GraphicsManager->GetPrevFrameID()];
    Texture* taaOutput = historyBuffers[g_GraphicsManager->GetCurrentFrameID()];
    Texture* mainDepth = graph.GetTexture(RESOURCE_ID(TextureID::MainDepth));

    glm::vec4 taaSettings{ g_GraphicsManager->GetGraphicsSettings().m_AlphaTAA, g_GraphicsManager->GetGraphicsSettings().m_VarianceGammaTAA, 0.0f, 0.0f };
    UniformProxy uniform = graph.AllocateUniform(GetID(), context, sizeof(taaSettings));
    uniform.WriteMember140(taaSettings);

    PipelineEntry* pipelineEntry = g_GraphicsManager->GetPipelineDB().GetEntry("temporal_AA");

    ResourceBinder binder = g_GraphicsManager->CreateResourceBinder(pipelineEntry, 0);
    binder.AddUniform(0, &uniform);
    binder.AddSampledTexture(1, velocity);
    binder.AddSampledTexture(2, colorInput);
    binder.AddSampledTexture(3, history);
    binder.AddStorageTexture(4, taaOutput);
    binder.AddSampledTexture(5, mainDepth);
    binder.FlushDescriptorWrites();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, colorInput->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, velocity->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, history->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, taaOutput->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, mainDepth->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);

    context.CmdBindComputeDescriptorSets(pipelineEntry->GetLayout(), binder.GetTargetSetID(), 1, &binder.GetDescriptorSet());
    context.CmdBindComputePipeline(pipelineEntry->GetPipeline());

    glm::uvec2 rtSize{ g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth, g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight };
    glm::uvec2 const groupSize{ 8, 8 };
    glm::uvec2 const dispatchSize = rtSize / groupSize + glm::uvec2{ 1, 1 };
    context.CmdDispatch(dispatchSize.x, dispatchSize.y, 1);
}

}
