#include <gfx\pass\ForwardOpaquePass.hpp>

#include <vk_wrapper\pipeline\ShaderModule.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>
#include <gfx\renderer\DrawBatcher.hpp>

#include <engine\io\IOManager.hpp>
#include <engine\scene\Scene.hpp>

namespace GFX
{

PassID GFX::ForwardOpaquePass::GetID() const
{
    return PassID::ForwardOpaque;
}

void ForwardOpaquePass::RegisterResources(RenderGraph& graph)
{
    graph.RegisterStorageTexture(this,
        TextureID::ShadowMap,
        VKW::FORMAT_D16_UNORM, C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_HEIGHT, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_FRAGMENT, 0);

    graph.RegisterUniformBuffer(this, VKW::STAGE_FRAGMENT, 1);
    
    graph.RegisterRenderTarget(this,
        TextureID::FinalRT,
        VKW::FORMAT_B8G8R8A8_UNORM, g_GraphicsManager->GetRenderingWidth(), g_GraphicsManager->GetRenderingHeight(),
        0);

    graph.RegisterDepthOnlyTarget(this,
        TextureID::MainDepth,
        VKW::FORMAT_D32_FLOAT, g_GraphicsManager->GetRenderingWidth(), g_GraphicsManager->GetRenderingHeight());
}

void ForwardOpaquePass::Initialize(RenderGraph& graph)
{
}

void ForwardObjectDelegate(RenderableObject& obj, VKW::Context& context, VKW::DescriptorManager& descriptorManager, UniformArena& arena, RenderView const& view)
{
    std::uint32_t constexpr uniformSize =
        sizeof(glm::mat4) * 2 +
        sizeof(VKW::TextureDescriptorIndex) * 4;

    auto uniformAllocation = arena.AllocateTransientRegion(g_GraphicsManager->GetCurrentFrameID(), uniformSize, 256);
    VKW::DescriptorManager::WriteDesc writeDesc;
    writeDesc.AddUniform(uniformAllocation.m_Buffer, uniformAllocation.m_OffsetInBuffer, uniformAllocation.m_Size, 0);
    descriptorManager.WriteDescriptorSet(obj.GetDescriptorSets(g_GraphicsManager->GetCurrentFrameID()), writeDesc);

    UniformProxy uniformProxy{ &context, uniformAllocation };

    //how to fucking write shadow uniform

    glm::mat4 const mvp = view.GetViewProjectionM() * obj.GetModelM();
    uniformProxy.WriteMember140(obj.GetModelM());
    uniformProxy.WriteMember140(mvp);

    std::uint32_t textureIDs[4] = {
       obj.GetDiffuseTexture()->GetShaderReadDescriptor().id_,
       obj.GetNormalTexture()->GetShaderReadDescriptor().id_,
       obj.GetMetalnessTexture()->GetShaderReadDescriptor().id_,
       obj.GetRoughnessTexture()->GetShaderReadDescriptor().id_
    };

    uniformProxy.WriteMember140(textureIDs, sizeof(textureIDs));
}

void ForwardOpaquePass::Render(RenderGraph& graph, VKW::Context& context)
{
    VKW::ImageResourceView* colorAttachment = graph.GetTexture(TextureID::FinalRT)->GetShaderView();
    VKW::ImageResourceView* depthAttachment = graph.GetTexture(TextureID::MainDepth)->GetShaderView();
    VKW::ImageResourceView* shadowMap       = graph.GetTexture(TextureID::ShadowMap)->GetShaderView();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, colorAttachment->parentResource_, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, depthAttachment->parentResource_, VKW::RESOURCE_ACCESS_DEPTH_ONLY_ATTACHMENT, VKW::STAGE_ALL_GRAPHICS);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, shadowMap->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_FRAGMENT);

    context.CmdBeginRendering(1, colorAttachment, depthAttachment, nullptr);
    float clearColors[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context.CmdClearAttachments(VKW::ATTACHMENT_MASK_COLOR_0, clearColors);
    context.CmdClearAttachments(VKW::ATTACHMENT_MASK_DEPTH, 0.0f, 0);

    context.CmdSetViewport(1, 0, 0, g_GraphicsManager->GetRenderingWidth(), g_GraphicsManager->GetRenderingHeight());
    context.CmdSetScissor(1, 0, 0, g_GraphicsManager->GetRenderingWidth(), g_GraphicsManager->GetRenderingHeight());

    {
        glm::mat4 const shadow_ViewProj = g_GraphicsManager->GetSunShadowRenderView().GetViewProjectionM();
        UniformProxy passUniformProxy = graph.GetPassUniform(GetID(), context, sizeof(shadow_ViewProj));
        passUniformProxy.WriteMember140(shadow_ViewProj);
    }

    VKW::DescriptorSet passSet = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());
    std::uint32_t const passSetBinding = g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->GetGlobalSetLayoutsCount();

    VKW::PipelineLayout* passLayout = graph.GetPassPipelineLayout(GetID());
    context.CmdBindDescriptorSets(passLayout, VKW::BindPoint::Graphics, passSetBinding, 1, &passSet);

    // 1. take all RenderableObject's in main scene
    DrawBatcher batcher{ &DRE::g_FrameScratchAllocator, g_GraphicsManager->GetMainDevice()->GetDescriptorManager(), &g_GraphicsManager->GetUniformArena() };
    batcher.Batch(context, g_GraphicsManager->GetMainRenderView(), GFX::ForwardObjectDelegate);

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
