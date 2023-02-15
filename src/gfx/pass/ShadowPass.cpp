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

static constexpr std::uint32_t C_SHADOW_MAP_WIDTH = 1024;
static constexpr std::uint32_t C_SHADOW_MAP_HEIGHT = 1024;


void ShadowPass::RegisterResources(RenderGraph& graph)
{
    graph.RegisterDepthStencilTarget(this,
        TextureID::ShadowMap,
        VKW::FORMAT_D16_UNORM, C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_WIDTH);
}

void ShadowPass::Render(RenderGraph& graph, VKW::Context& context)
{
    VKW::ImageResourceView* depthAttachment = graph.GetStorageTexture(TextureID::ShadowMap)->GetShaderView();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, depthAttachment->parentResource_, VKW::RESOURCE_ACCESS_DEPTH_STENCIL_ATTACHMENT, VKW::STAGE_ALL_GRAPHICS);
    context.CmdBeginRendering(0, nullptr, depthAttachment, nullptr);
    context.CmdClearAttachments(VKW::ATTACHMENT_MASK_DEPTH, 1.0f, 0);

    context.CmdSetViewport(1, 0, 0, C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_HEIGHT);
    context.CmdSetScissor(1, 0, 0, C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_HEIGHT);

    // 1. take all RenderableObject's in main scene
    DrawBatcher batcher{ &DRE::g_FrameScratchAllocator, g_GraphicsManager->GetMainDevice()->GetDescriptorManager(), &g_GraphicsManager->GetUniformArena() };
    auto& allEntities = WORLD::g_MainScene->GetEntities();
    for (std::uint32_t i = 0, size = allEntities.Size(); i < size; i++)
    {
        batcher.AddRenderable(allEntities[i].GetRenderableObject());
    }

    batcher.Batch(context, *WORLD::g_MainScene);

    std::uint32_t const startSet = 
        g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->GetGlobalSetLayoutsCount() +
        (graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID()).IsValid() ? 1 : 0);

    auto& draws = batcher.GetOpaqueDraws();
    for (std::uint32_t i = 0, size = draws.Size(); i < size; i++)
    {
        AtomDraw const& atom = draws[i];
        context.CmdBindGraphicsPipeline(atom.pipeline);
        context.CmdBindGraphicsDescriptorSets(atom.pipeline->GetLayout(), startSet, 1, &atom.descriptorSet);
        context.CmdBindVertexBuffer(atom.vertexBuffer, atom.vertexOffset);
        context.CmdBindIndexBuffer(atom.indexBuffer, atom.indexOffset);
        context.CmdDrawIndexed(atom.indexCount);
    }

    // 2. feed all RenderableObject's to DrawBatcher
    // 3. DrawBatcher produces atomic draw commands
    context.CmdEndRendering();
}

}
