#include <gfx\pass\AmbientOcclusionPass.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>

namespace GFX
{

PassID AmbientOcclusionPass::GetID() const
{
    return PassID::AmbientOcclusion;
}

void AmbientOcclusionPass::Initialize(RenderGraph& graph)
{
}

void AmbientOcclusionPass::RegisterResources(RenderGraph& graph)
{
    std::uint32_t renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth, renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

    VKW::Format const depthFormat = g_GraphicsManager->GetMainDepthFormat();
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::AmbientOcclusion), VKW::FORMAT_R8_UNORM, renderWidth, renderHeight, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE, 0);
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::MainDepth), depthFormat, renderWidth, renderHeight, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE, 1);
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::GBufferB_NormalMetalness), g_GraphicsManager->GetGBufferFormats()[1], renderWidth, renderHeight, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE, 2);

    graph.RegisterUniformBuffer(this, VKW::STAGE_COMPUTE, 3);
}

void AmbientOcclusionPass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(AmbientOcclusion);

    VKW::ImageResourceView* aoOutput = graph.GetTexture(RESOURCE_ID(TextureID::AmbientOcclusion))->GetShaderView();
    VKW::ImageResourceView* depth = graph.GetTexture(RESOURCE_ID(TextureID::MainDepth))->GetShaderView();
    VKW::ImageResourceView* gBufferNormal_Metalness = graph.GetTexture(RESOURCE_ID(TextureID::GBufferB_NormalMetalness))->GetShaderView();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, aoOutput->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, depth->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, gBufferNormal_Metalness->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);

    UniformProxy uniform = graph.GetPassUniform(GetID(), context, 128);

    float const aoStrength = g_GraphicsManager->GetGraphicsSettings().m_AOStrength;
    float const aoKernelScale = g_GraphicsManager->GetGraphicsSettings().m_AOKernelScale;
    uniform.WriteMember140(glm::vec4{ aoStrength, aoKernelScale, 0.0f, 0.0f });


    VKW::PipelineLayout* layout = graph.GetPassPipelineLayout(GetID());
    VKW::Pipeline* pipeline = g_GraphicsManager->GetPipelineDB().GetPipeline("ambient_occlusion");

    VKW::DescriptorSet passDescriptorSet = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());
    context.CmdBindComputeDescriptorSets(layout, graph.GetPassSetBinding(), 1, &passDescriptorSet);
    context.CmdBindComputePipeline(pipeline);

    glm::uvec2 rtSize{ g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth, g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight };
    glm::uvec2 const groupSize{ 8, 8 };
    glm::uvec2 const dispatchSize = rtSize / groupSize + glm::uvec2{ 1, 1 };
    context.CmdDispatch(dispatchSize.x, dispatchSize.y, 1);
}

}
