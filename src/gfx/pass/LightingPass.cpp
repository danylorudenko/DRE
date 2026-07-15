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

    glm::uvec2 ddgiAtlasDims = g_GraphicsManager->GetDDGI().GetProbeAtlasIrradianceDimentions();

    graph.RegisterTexture(this,
        RESOURCE_ID(TextureID::ForwardColor),
        g_GraphicsManager->GetMainColorFormat(), renderWidth, renderHeight,
        VKW::RESOURCE_ACCESS_SHADER_WRITE);

    graph.RegisterTexture(this,
        RESOURCE_ID(TextureID::GBufferA_DiffuseRoughness),
        gBufferFormats[0], renderWidth, renderHeight,
        VKW::RESOURCE_ACCESS_SHADER_SAMPLE);

    graph.RegisterTexture(this,
        RESOURCE_ID(TextureID::GBufferB_NormalMetalness),
        gBufferFormats[1], renderWidth, renderHeight,
        VKW::RESOURCE_ACCESS_SHADER_SAMPLE);

    graph.RegisterTexture(this,
        RESOURCE_ID(TextureID::MainDepth),
        g_GraphicsManager->GetMainDepthFormat(), renderWidth, renderHeight,
        VKW::RESOURCE_ACCESS_SHADER_SAMPLE);

    graph.RegisterTexture(this,
        RESOURCE_ID(TextureID::AmbientOcclusion),
        VKW::FORMAT_R8_UNORM, renderWidth, renderHeight,
        VKW::RESOURCE_ACCESS_SHADER_SAMPLE);

    // it's implicitly accessed from global uniform
    graph.RegisterTexture(this,
        RESOURCE_ID(TextureID::DDGI_ProbeIrradiance),
        VKW::FORMAT_R16G16B16A16_FLOAT, ddgiAtlasDims.x, ddgiAtlasDims.y,
        VKW::RESOURCE_ACCESS_SHADER_SAMPLE, GraphResourceFlags::INIT_CLEAR);
}

void LightingPass::Initialize(RenderGraph&)
{
}

void LightingPass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(Lighting);

    Texture* forwardColor = graph.GetTexture(RESOURCE_ID(TextureID::ForwardColor));

    Texture* gbufferA = graph.GetTexture(RESOURCE_ID(TextureID::GBufferA_DiffuseRoughness));
    Texture* gbufferB = graph.GetTexture(RESOURCE_ID(TextureID::GBufferB_NormalMetalness));
    Texture* depth = graph.GetTexture(RESOURCE_ID(TextureID::MainDepth));
    Texture* ambientOcclusion = graph.GetTexture(RESOURCE_ID(TextureID::AmbientOcclusion));
    Texture* ddgiIrradiance = graph.GetTexture(RESOURCE_ID(TextureID::DDGI_ProbeIrradiance)); // implicity accessed from global uniform

    auto& dependencyManager = g_GraphicsManager->GetDependencyManager();
    dependencyManager.ResourceBarrier(context, forwardColor->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);

    dependencyManager.ResourceBarrier(context, gbufferA->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);
    dependencyManager.ResourceBarrier(context, gbufferB->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);
    dependencyManager.ResourceBarrier(context, depth->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);
    dependencyManager.ResourceBarrier(context, ambientOcclusion->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);
    dependencyManager.ResourceBarrier(context, ddgiIrradiance->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);

    PipelineEntry* entry = g_GraphicsManager->GetPipelineDB().GetEntry("lighting_deferred");
    ResourceBinder binder = g_GraphicsManager->CreateResourceBinder(entry, 0);
    binder.AddStorageTexture(0, forwardColor);
    binder.AddSampledTexture(1, gbufferA);
    binder.AddSampledTexture(2, gbufferB);
    binder.AddSampledTexture(3, depth);
    binder.AddSampledTexture(4, ambientOcclusion);
    binder.FlushDescriptorWrites();

    context.CmdBindComputeDescriptorSets(entry->GetLayout(), binder.GetTargetSetID(), 1, &binder.GetDescriptorSet());
    context.CmdBindComputePipeline(entry->GetPipeline());

    std::uint32_t renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth;
    std::uint32_t renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

    constexpr std::uint32_t groupSizeX = 8;
    constexpr std::uint32_t groupSizeY = 8;
    std::uint32_t dispatchX = (renderWidth + groupSizeX - 1) / groupSizeX;
    std::uint32_t dispatchY = (renderHeight + groupSizeY - 1) / groupSizeY;

    context.CmdDispatch(dispatchX, dispatchY, 1);
}

}
