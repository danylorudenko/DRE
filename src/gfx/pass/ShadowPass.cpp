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
        TextureID::CausticEnvMap,
        VKW::FORMAT_R16G16B16A16_FLOAT, C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_HEIGHT, 0);

    graph.RegisterDepthOnlyTarget(this,
        TextureID::ShadowMap,
        VKW::FORMAT_D16_UNORM, C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_WIDTH);
}


void ShadowObjectDelegate(RenderableObject& obj, VKW::Context& context, VKW::DescriptorManager& descriptorManager, UniformArena& arena, RenderView const& view)
{
    std::uint32_t constexpr uniformSize = sizeof(glm::mat4) * 2;

    auto uniformAllocation = arena.AllocateTransientRegion(g_GraphicsManager->GetCurrentFrameID(), uniformSize, 256);
    VKW::DescriptorManager::WriteDesc writeDesc;
    writeDesc.AddUniform(uniformAllocation.m_Buffer, uniformAllocation.m_OffsetInBuffer, uniformAllocation.m_Size, 0);
    descriptorManager.WriteDescriptorSet(obj.GetShadowDescriptorSet(g_GraphicsManager->GetCurrentFrameID()), writeDesc);

    UniformProxy uniformProxy{ &context, uniformAllocation };

    glm::mat4 const world = obj.GetSceneNode()->GetGlobalMatrix();
    glm::mat4 const mvp = view.GetViewProjectionM() * world;
    uniformProxy.WriteMember140(mvp);
    uniformProxy.WriteMember140(world);
}


void ShadowPass::Render(RenderGraph& graph, VKW::Context& context)
{
    VKW::ImageResourceView* wposAttachment = graph.GetTexture(TextureID::CausticEnvMap)->GetShaderView();
    VKW::ImageResourceView* depthAttachment = graph.GetTexture(TextureID::ShadowMap)->GetShaderView();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, wposAttachment->parentResource_, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, depthAttachment->parentResource_, VKW::RESOURCE_ACCESS_DEPTH_ONLY_ATTACHMENT, VKW::STAGE_ALL_GRAPHICS);

    context.CmdBeginRendering(1, &wposAttachment, depthAttachment, nullptr);
    float clearValues[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context.CmdClearAttachments(VKW::ATTACHMENT_MASK_COLOR_0, clearValues);
    context.CmdClearAttachments(VKW::ATTACHMENT_MASK_DEPTH, 0.0f, 0);

    context.CmdSetViewport(1, 0, 0, C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_HEIGHT);
    context.CmdSetScissor(1, 0, 0, C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_HEIGHT);
#ifndef DRE_COMPILE_FOR_RENDERDOC
    context.CmdSetPolygonMode(VKW::POLYGON_FILL);
#endif // DRE_COMPILE_FOR_RENDERDOC

    // 1. take all RenderableObject's in main scene
    DrawBatcher batcher{ &DRE::g_FrameScratchAllocator, g_GraphicsManager->GetMainDevice()->GetDescriptorManager(), &g_GraphicsManager->GetUniformArena() };

    batcher.BatchShadow(context, g_GraphicsManager->GetSunShadowRenderView(), RenderableObject::LAYER_OPAQUE_BIT, GFX::ShadowObjectDelegate);

    std::uint32_t const startSet = 
        g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->GetGlobalSetLayoutsCount() +
        (graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID()).IsValid() ? 1 : 0);

    auto& draws = batcher.GetDraws();
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
