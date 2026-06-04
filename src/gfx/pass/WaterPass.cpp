#include <gfx\pass\WaterPass.hpp>

#include <vk_wrapper\pipeline\ShaderModule.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>
#include <gfx\renderer\DrawBatcher.hpp>

#include <engine\io\IOManager.hpp>
#include <engine\scene\Scene.hpp>

namespace GFX
{

PassID GFX::WaterPass::GetID() const
{
    return PassID::Water;
}

void WaterPass::RegisterResources(RenderGraph& graph)
{
    /*
    std::uint32_t renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth, renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

    graph.RegisterTexture(this,
        RESOURCE_ID(TextureID::ShadowMap),
        VKW::FORMAT_D16_UNORM, C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_HEIGHT, VKW::RESOURCE_ACCESS_SHADER_SAMPLE);

    graph.RegisterTexture(this,
        RESOURCE_ID(TextureID::ForwardColor),
        g_GraphicsManager->GetMainColorFormat(), renderWidth, renderHeight,
        VKW::RESOURCE_ACCESS_SHADER_SAMPLE);

    graph.RegisterTexture(this,
        RESOURCE_ID(TextureID::MainDepth),
        g_GraphicsManager->GetMainDepthFormat(), renderWidth, renderHeight,
        VKW::RESOURCE_ACCESS_SHADER_SAMPLE);

    graph.RegisterTexture(this,
        RESOURCE_ID(TextureID::WaterHeight),
        VKW::FORMAT_R32_FLOAT, C_WATER_DIM, C_WATER_DIM,
        VKW::RESOURCE_ACCESS_SHADER_SAMPLE);

    graph.RegisterRenderTarget(this,
        RESOURCE_ID(TextureID::WaterColor),
        g_GraphicsManager->GetMainColorFormat(), renderWidth, renderHeight,
        0);

    graph.RegisterRenderTarget(this,
        RESOURCE_ID(TextureID::GBufferC_Velocity),
        g_GraphicsManager->GetVelocityBufferFormat(), renderWidth, renderHeight,
        1);

    graph.RegisterDepthOnlyTarget(this,
        RESOURCE_ID(TextureID::MainDepth),
        g_GraphicsManager->GetMainDepthFormat(), renderWidth, renderHeight);
    */
}

void WaterPass::Initialize(RenderGraph& graph)
{
}

void WaterPass::Render(RenderGraph& graph, VKW::Context& context)
{
    /*
    DRE_GPU_SCOPE(Water);

    Texture* waterAttachment = graph.GetTexture(RESOURCE_ID(TextureID::WaterColor));
    Texture* velocityAttachment = graph.GetTexture(RESOURCE_ID(TextureID::GBufferC_Velocity));
    Texture* depthAttachment = graph.GetTexture(RESOURCE_ID(TextureID::MainDepth));
    Texture* shadowMap       = graph.GetTexture(RESOURCE_ID(TextureID::ShadowMap));
    Texture* heightMap       = graph.GetTexture(RESOURCE_ID(TextureID::WaterHeight));
    Texture* color           = graph.GetTexture(RESOURCE_ID(TextureID::ForwardColor));

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, waterAttachment->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_TRANSFER_DST, VKW::STAGE_TRANSFER);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, color->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_TRANSFER_SRC, VKW::STAGE_TRANSFER);

    context.CmdCopyImageToImage(waterAttachment->GetShaderView()->parentResource_, color->GetShaderView()->parentResource_);

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, waterAttachment->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, velocityAttachment->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, depthAttachment->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_ALL_GRAPHICS); // but also used as depth readonly attachment
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, shadowMap->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_FRAGMENT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, heightMap->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_VERTEX);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, color->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_FRAGMENT);

    PipelineEntry* passEntry = g_GraphicsManager->GetPipelineDB().GetEntry("water");

    VKW::ImageResourceView* attachments[2] = { waterAttachment->GetShaderView(), velocityAttachment->GetShaderView() };

    DrawBatcher batcher{ &DRE::g_FrameScratchAllocator, g_GraphicsManager->GetMainDevice()->GetDescriptorManager(), &g_GraphicsManager->GetUniformArena() };
    batcher.Batch(context, g_GraphicsManager->GetMainRenderView(), passLayout, RenderableObject::LAYER_WATER, nullptr);

    context.CmdBeginRendering(2, attachments, depthAttachment->GetShaderView(), nullptr);

    std::uint32_t renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth, renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;
    context.CmdSetViewport(2, 0, 0, renderWidth, renderHeight);
    context.CmdSetScissor(2, 0, 0, renderWidth, renderHeight);
#ifndef DRE_COMPILE_FOR_RENDERDOC
    context.CmdSetPolygonMode(g_GraphicsManager->GetGraphicsSettings().m_WaterWireframe ? VKW::POLYGON_WIREFRAME : VKW::POLYGON_FILL);
#endif // DRE_COMPILE_FOR_RENDERDOC

    {
        glm::mat4 const shadow_ViewProj = g_GraphicsManager->GetSunShadowRenderView().GetViewProjectionM();
        glm::vec4 const shadow_Size = glm::vec4{ C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_HEIGHT, 0.0f, 0.0f };
        glm::vec4 const useFFT = glm::vec4{ g_GraphicsManager->GetGraphicsSettings().m_UseFFTWater ? 1.0f : 0.0f, C_WATER_VERTEX_X, C_WATER_VERTEX_Z, g_GraphicsManager->GetGraphicsSettings().m_WindDirectionX };

        std::uint32_t constexpr passUniformSize = sizeof(shadow_ViewProj) + sizeof(shadow_Size) + sizeof(useFFT);

        UniformProxy passUniformProxy = graph.GetPassUniform(GetID(), context, passUniformSize);
        passUniformProxy.WriteMember140(shadow_ViewProj);
        passUniformProxy.WriteMember140(shadow_Size);
        passUniformProxy.WriteMember140(useFFT);
    }

    VKW::DescriptorSet passSet = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());
    std::uint32_t const passSetBinding = VKW::DescriptorManager::GLOBAL_SET_COUNT;

    context.CmdBindGraphicsDescriptorSets(passLayout, passSetBinding, 1, &passSet);

    std::uint32_t const startSet = graph.GetUserSetBinding(GetID());

    auto& draws = batcher.GetDraws();
    for (std::uint32_t i = 0, size = draws.Size(); i < size; i++)
    {
        AtomDraw const& atom = draws[i];
        context.CmdBindGraphicsPipeline(atom.pipeline);
        // push constant here?
        //context.CmdBindGraphicsDescriptorSets(atom.pipeline->GetLayout(), startSet, 1, &atom.descriptorSet);
        context.CmdBindVertexBuffer(atom.vertexBuffer, atom.vertexOffset);
        context.CmdBindIndexBuffer(atom.indexBuffer, atom.indexOffset);
        context.CmdDrawIndexed(atom.indexCount);
    }

    context.CmdEndRendering();

    */
}

}
