#include <gfx\pass\ShadowPass.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>
#include <gfx\renderer\DrawBatcher.hpp>

#include <engine\scene\Scene.hpp>

namespace GFX
{

PassID ShadowPass::GetID() const
{
    return PassID::Shadow;
}

void ShadowPass::Initialize(RenderGraph& graph)
{
}

void ShadowPass::RegisterResources(RenderGraph& graph)
{
    graph.RegisterRenderTarget(this,
        RESOURCE_ID(TextureID::CausticEnvMap),
        VKW::FORMAT_R16G16B16A16_FLOAT, C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_HEIGHT, 0);

    graph.RegisterDepthOnlyTarget(this,
        RESOURCE_ID(TextureID::ShadowMap),
        VKW::FORMAT_D16_UNORM, C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_WIDTH);
}

void ShadowPass::Render(RenderGraph& graph, VKW::Context& context)
{
    // not functional
    return;

    DRE_GPU_SCOPE(Shadow);

    Texture* wposAttachment = graph.GetTexture(RESOURCE_ID(TextureID::CausticEnvMap));
    Texture* depthAttachment = graph.GetTexture(RESOURCE_ID(TextureID::ShadowMap));

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, wposAttachment->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, depthAttachment->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_DEPTH_ONLY_ATTACHMENT, VKW::STAGE_ALL_GRAPHICS);

    VKW::ImageResourceView* wposAttachmentView = wposAttachment->GetShaderView();
    context.CmdBeginRendering(1, &wposAttachmentView, depthAttachment->GetShaderView(), nullptr);
    float clearValues[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context.CmdClearAttachments(VKW::ATTACHMENT_MASK_COLOR_0, clearValues);
    context.CmdClearAttachments(VKW::ATTACHMENT_MASK_DEPTH, 0.0f, 0);

    context.CmdSetViewport(1, 0, 0, C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_HEIGHT);
    context.CmdSetScissor(1, 0, 0, C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_HEIGHT);
#ifndef DRE_COMPILE_FOR_RENDERDOC
    context.CmdSetPolygonMode(VKW::POLYGON_FILL);
#endif // DRE_COMPILE_FOR_RENDERDOC

    PipelineEntry* entry = g_GraphicsManager->GetPipelineDB().GetEntry("forward_shadow");

    // 1. take all RenderableObject's in main scene
    DrawBatcher batcher{ &DRE::g_FrameScratchAllocator, g_GraphicsManager->GetMainDevice()->GetDescriptorManager(), &g_GraphicsManager->GetUniformArena() };

    //batcher.Batch(context, g_GraphicsManager->GetSunShadowRenderView(), graph.GetPassPipelineLayout(GetID()), RenderableObject::LAYER_SHADOW, nullptr);

    //std::uint32_t const startSet = graph.GetUserSetBinding(GetID());

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

    // 2. feed all RenderableObject's to DrawBatcher
    // 3. DrawBatcher produces atomic draw commands
    context.CmdEndRendering();
}

}
