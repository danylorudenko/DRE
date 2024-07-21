#include <gfx\pass\ForwardOpaquePass.hpp>

#include <vk_wrapper\pipeline\ShaderModule.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>
#include <gfx\renderer\DrawBatcher.hpp>

#include <engine\io\IOManager.hpp>
#include <engine\scene\Scene.hpp>

#include <forward.h>

namespace GFX
{

PassID GFX::ForwardOpaquePass::GetID() const
{
    return PassID::ForwardOpaque;
}

void ForwardOpaquePass::RegisterResources(RenderGraph& graph)
{
    std::uint32_t renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth, renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

    graph.RegisterTexture(this,
        RESOURCE_ID(TextureID::ShadowMap),
        VKW::FORMAT_D16_UNORM, C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_HEIGHT, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_FRAGMENT, 0);

    graph.RegisterUniformBuffer(this, VKW::STAGE_FRAGMENT, 1);

    graph.RegisterTexture(this,
        RESOURCE_ID(TextureID::CausticMap),
        VKW::FORMAT_R8_UNORM, C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_HEIGHT, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_FRAGMENT, 2);


    graph.RegisterRenderTarget(this,
        RESOURCE_ID(TextureID::ForwardColor),
        g_GraphicsManager->GetMainColorFormat(), renderWidth, renderHeight,
        0);

    graph.RegisterRenderTarget(this,
        RESOURCE_ID(TextureID::Velocity),
        VKW::FORMAT_R16G16_FLOAT, renderWidth, renderHeight,
        1);

    graph.RegisterRenderTarget(this,
        RESOURCE_ID(TextureID::ObjectIDBuffer),
        VKW::FORMAT_R32_UINT, renderWidth, renderHeight,
        2);

    graph.RegisterDepthOnlyTarget(this,
        RESOURCE_ID(TextureID::MainDepth),
        g_GraphicsManager->GetMainDepthFormat(), renderWidth, renderHeight);
}

void ForwardOpaquePass::Initialize(RenderGraph& graph)
{
}

void ForwardObjectDelegate(RenderableObject& obj, VKW::Context& context, VKW::DescriptorManager& descriptorManager, UniformArena& arena, RenderView const& view)
{
    std::uint32_t constexpr uniformSize =
        sizeof(glm::mat4) * 2 +
        sizeof(VKW::TextureDescriptorIndex::id_) * 4;

    auto uniformAllocation = arena.AllocateTransientRegion(g_GraphicsManager->GetCurrentFrameID(), uniformSize, 256);
    VKW::DescriptorManager::WriteDesc writeDesc;
    writeDesc.AddUniform(uniformAllocation.m_Buffer, uniformAllocation.m_OffsetInBuffer, uniformAllocation.m_Size, 0);
    descriptorManager.WriteDescriptorSet(obj.GetDescriptorSet(g_GraphicsManager->GetCurrentFrameID()), writeDesc);

    UniformProxy uniformProxy{ &context, uniformAllocation };
    glm::mat4 worldMatrix = obj.GetSceneNode()->GetGlobalMatrix();
    uniformProxy.WriteMember140(worldMatrix);
    uniformProxy.WriteMember140(worldMatrix); // prev world matrix is same, geometry is static

    std::uint32_t textureIDs[4] = {
       obj.GetDiffuseTexture()->GetShaderGlobalDescriptor().id_,
       obj.GetNormalTexture()->GetShaderGlobalDescriptor().id_,
       obj.GetMetalnessTexture()->GetShaderGlobalDescriptor().id_,
       obj.GetRoughnessTexture()->GetShaderGlobalDescriptor().id_
    };

    uniformProxy.WriteMember140(textureIDs, sizeof(textureIDs));
}

void ForwardOpaquePass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(ForwardOpaque);

    std::uint32_t renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth, renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

    VKW::ImageResourceView* colorAttachment = graph.GetTexture(RESOURCE_ID(TextureID::ForwardColor))->GetShaderView();
    VKW::ImageResourceView* velocityAttachment = graph.GetTexture(RESOURCE_ID(TextureID::Velocity))->GetShaderView();
    VKW::ImageResourceView* objectIDAttachment = graph.GetTexture(RESOURCE_ID(TextureID::ObjectIDBuffer))->GetShaderView();
    VKW::ImageResourceView* depthAttachment = graph.GetTexture(RESOURCE_ID(TextureID::MainDepth))->GetShaderView();
    VKW::ImageResourceView* shadowMap       = graph.GetTexture(RESOURCE_ID(TextureID::ShadowMap))->GetShaderView();
    VKW::ImageResourceView* causticMap      = graph.GetTexture(RESOURCE_ID(TextureID::CausticMap))->GetShaderView();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, colorAttachment->parentResource_, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, velocityAttachment->parentResource_, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, objectIDAttachment->parentResource_, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, depthAttachment->parentResource_, VKW::RESOURCE_ACCESS_DEPTH_ONLY_ATTACHMENT, VKW::STAGE_ALL_GRAPHICS);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, shadowMap->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_FRAGMENT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, causticMap->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_FRAGMENT);

    std::uint32_t constexpr attachmentsCount = 3;
    static_assert(FORWARD_PASS_OUTPUT_COUNT == attachmentsCount, "Don't forget to modify PipelineDB and ForwardOpaquePass");
    VKW::ImageResourceView* attachments[attachmentsCount] = { colorAttachment, velocityAttachment, objectIDAttachment };

    DrawBatcher batcher{ &DRE::g_FrameScratchAllocator, g_GraphicsManager->GetMainDevice()->GetDescriptorManager(), &g_GraphicsManager->GetUniformArena() };
    batcher.Batch(context, g_GraphicsManager->GetMainRenderView(), RenderableObject::LAYER_OPAQUE_BIT, GFX::ForwardObjectDelegate);


    context.CmdBeginRendering(attachmentsCount, attachments, depthAttachment, nullptr);
    float clearColors[4] = { 0.9f, 0.9f, 0.9f, 0.0f };
    float clearVelocity[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    std::uint32_t clearID[4] = { 0, 0, 0, 0 };
    context.CmdClearAttachments(VKW::ATTACHMENT_MASK_COLOR_0, clearColors);
    context.CmdClearAttachments(VKW::ATTACHMENT_MASK_COLOR_1, clearVelocity);
    context.CmdClearAttachments(VKW::ATTACHMENT_MASK_COLOR_2, clearID);
    context.CmdClearAttachments(VKW::ATTACHMENT_MASK_DEPTH, 0.0f, 0);

    context.CmdSetViewport(attachmentsCount, 0, 0, renderWidth, renderHeight);
    context.CmdSetScissor(attachmentsCount, 0, 0, renderWidth, renderHeight);
#ifndef DRE_COMPILE_FOR_RENDERDOC
    context.CmdSetPolygonMode(VKW::POLYGON_FILL);
#endif // DRE_COMPILE_FOR_RENDERDOC

    {
        glm::mat4 const shadow_ViewProj = g_GraphicsManager->GetSunShadowRenderView().GetViewProjectionM();
        glm::vec4 const shadow_Size = glm::vec4{ C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_HEIGHT, 0.0f, 0.0f };

        std::uint32_t constexpr passUniformSize = sizeof(shadow_ViewProj) + sizeof(shadow_Size);

        UniformProxy passUniformProxy = graph.GetPassUniform(GetID(), context, passUniformSize);
        ForwardUniform passData;
        passData.shadow_VP = shadow_ViewProj;
        passData.shadow_size = shadow_Size;
        passUniformProxy.WriteMember140(passData);
    }

    VKW::DescriptorSet passSet = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());
    std::uint32_t const passSetBinding = g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->GetGlobalSetLayoutsCount();

    VKW::PipelineLayout* passLayout = graph.GetPassPipelineLayout(GetID());
    context.CmdBindDescriptorSets(passLayout, VKW::BindPoint::Graphics, passSetBinding, 1, &passSet);

    std::uint32_t const startSet = 
        g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->GetGlobalSetLayoutsCount() +
        (graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID()).IsValid() ? 1 : 0);

    auto& draws = batcher.GetDraws();

    VKW::Pipeline* prevPipeline = nullptr;
    for (std::uint32_t i = 0, size = draws.Size(); i < size; i++)
    {
        AtomDraw const& atom = draws[i];
        if (prevPipeline != atom.pipeline)
        {
            context.CmdBindGraphicsPipeline(atom.pipeline);
            prevPipeline = atom.pipeline;
        }

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
