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
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::AmbientOcclusion), VKW::FORMAT_R8_UNORM, renderWidth, renderHeight, VKW::RESOURCE_ACCESS_SHADER_WRITE);
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::MainDepth), depthFormat, renderWidth, renderHeight, VKW::RESOURCE_ACCESS_SHADER_SAMPLE);
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::GBufferB_NormalMetalness), g_GraphicsManager->GetGBufferFormats()[1], renderWidth, renderHeight, VKW::RESOURCE_ACCESS_SHADER_SAMPLE);
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::DEBUG_TEXTURE), VKW::FORMAT_R32G32B32A32_FLOAT, renderWidth, renderHeight, VKW::RESOURCE_ACCESS_SHADER_RW);
}

void AmbientOcclusionPass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(AmbientOcclusion);

    VKW::ImageResourceView* aoOutput = graph.GetTexture(RESOURCE_ID(TextureID::AmbientOcclusion))->GetShaderView();
    VKW::ImageResourceView* depth = graph.GetTexture(RESOURCE_ID(TextureID::MainDepth))->GetShaderView();
    VKW::ImageResourceView* gBufferNormal_Metalness = graph.GetTexture(RESOURCE_ID(TextureID::GBufferB_NormalMetalness))->GetShaderView();
    VKW::ImageResourceView* DEBUG_TEXTURE = graph.GetTexture(RESOURCE_ID(TextureID::DEBUG_TEXTURE))->GetShaderView();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, aoOutput->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, depth->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, gBufferNormal_Metalness->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, DEBUG_TEXTURE->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);

    UniformProxy uniform = graph.AllocateUniform(GetID(), context, 128);

    float const aoStrength = g_GraphicsManager->GetGraphicsSettings().m_AOStrength;
    float const aoKernelScale = g_GraphicsManager->GetGraphicsSettings().m_AOKernelScale;
    float const aoMaxOcclusionDistance = g_GraphicsManager->GetGraphicsSettings().m_AOMaxOcclusionDistance;
    AOVersion const aoVersion = g_GraphicsManager->GetGraphicsSettings().m_AOVersion;
    uniform.WriteMember140(glm::vec4{ aoStrength, aoKernelScale, aoMaxOcclusionDistance, 0.0f });
    uniform.WriteMember140(glm::ivec4{ aoVersion, 0, 0, 0 });

    PipelineEntry* pipelineEntry = g_GraphicsManager->GetPipelineDB().GetEntry("ambient_occlusion");

    ResourceBinder binder = g_GraphicsManager->CreateResourceBinder(pipelineEntry, 0);

    context.CmdBindComputeDescriptorSets(pipelineEntry->GetLayout(), binder.GetTargetSetID(), 1, &binder.GetDescriptorSet());
    context.CmdBindComputePipeline(pipelineEntry->GetPipeline());

    glm::uvec2 rtSize{ g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth, g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight };
    glm::uvec2 const groupSize{ 8, 8 };
    glm::uvec2 const dispatchSize = rtSize / groupSize + glm::uvec2{ 1, 1 };
    context.CmdDispatch(dispatchSize.x, dispatchSize.y, 1);
}

}
