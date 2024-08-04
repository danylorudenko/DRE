#include <gfx\pass\ForwardOpaquePass.hpp>

#include <vk_wrapper\pipeline\ShaderModule.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>
#include <gfx\renderer\DrawBatcher.hpp>

#include <engine\io\IOManager.hpp>
#include <engine\scene\Scene.hpp>

#include <engine\ApplicationContext.hpp>

#include <forward.h>
#include <forward_output.h>

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
        VKW::FORMAT_D16_UNORM, C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_HEIGHT, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_VERTEX | VKW::STAGE_FRAGMENT, 0);

    graph.RegisterUniformBuffer(this, VKW::STAGE_FRAGMENT, 1);

    graph.RegisterTexture(this,
        RESOURCE_ID(TextureID::CausticMap),
        VKW::FORMAT_R8_UNORM, C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_HEIGHT, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_VERTEX | VKW::STAGE_FRAGMENT, 2);


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
        g_GraphicsManager->GetObjectIDBufferFormat(), renderWidth, renderHeight,
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
    std::uint32_t constexpr uniformSize = sizeof(InstanceUniform);

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

    uniformProxy.WriteMember140(obj.GetSceneNode()->GetGlobalID());
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
    context.CmdClearAttachments(VKW::ATTACHMENT_MASK_COLOR_1 | VKW::ATTACHMENT_MASK_COLOR_2, clearVelocity);
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

    VKW::PipelineLayout* passLayout = graph.GetPassPipelineLayout(GetID());
    context.CmdBindDescriptorSets(passLayout, VKW::BindPoint::Graphics, graph.GetPassSetBinding(), 1, &passSet);

    auto& draws = batcher.GetDraws();

    std::uint32_t const userSetBinding = graph.GetUserSetBinding(GetID());
    VKW::Pipeline* prevPipeline = nullptr;
    for (std::uint32_t i = 0, size = draws.Size(); i < size; i++)
    {
        AtomDraw const& atom = draws[i];
        if (prevPipeline != atom.pipeline)
        {
            context.CmdBindGraphicsPipeline(atom.pipeline);
            prevPipeline = atom.pipeline;
        }

        context.CmdBindGraphicsDescriptorSets(atom.pipeline->GetLayout(), userSetBinding, 1, &atom.descriptorSet);
        context.CmdBindVertexBuffer(atom.vertexBuffer, atom.vertexOffset);
        context.CmdBindIndexBuffer(atom.indexBuffer, atom.indexOffset);
        context.CmdDrawIndexed(atom.indexCount);
    }

    context.CmdEndRendering();

    ReadbackScheduler readback(g_GraphicsManager->GetCurrentFrameID(), &g_GraphicsManager->GetReadbackArena(), renderWidth * renderHeight * 4);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, objectIDAttachment->parentResource_, VKW::RESOURCE_ACCESS_TRANSFER_SRC, VKW::STAGE_TRANSFER);
    context.CmdCopyImageToBuffer(readback.GetDstBuffer(), objectIDAttachment->parentResource_, readback.GetDstOffset());

    // queue submit!!!
    ReadbackFuture tempFuture = readback.CreateReadbackFuture(context.SyncPoint());

    if (static_cast<bool>(m_LastObjectIDsFuture))
    {
        m_LastObjectIDsFuture.Sync();
        void* readbackData = m_LastObjectIDsFuture.GetMappedPtr();

        DRE::S32 x = DRE::Clamp(DRE::g_AppContext.m_CursorX, 0, DRE::S32(renderWidth - 1));
        DRE::S32 y = DRE::Clamp(DRE::g_AppContext.m_CursorY, 0, DRE::S32(renderHeight - 1));
        DRE::g_AppContext.m_PickedObjectID = ObjectIDFromBuffer(readbackData, x, y);
    }

    m_LastObjectIDsFuture = tempFuture;
    context.CmdBindGlobalDescriptorSets(*g_GraphicsManager->GetMainDevice()->GetDescriptorManager(), g_GraphicsManager->GetCurrentFrameID());
}

DRE::U32 ForwardOpaquePass::ObjectIDFromBuffer(void* ptr, DRE::U32 x, DRE::U32 y)
{
    DRE::U32 const xOffset = x * 4;
    DRE::U32 const yOffset = y * 4 * g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth;

    DRE::U32 const pixel = *(DRE::U32 const*)((DRE::U8 const*)ptr + (xOffset + yOffset));

    // layout: ARGB (from BGRA8)
    DRE::U32 const a = (pixel & 0xFF000000) >> 24;
    DRE::U32 const rgb = (pixel & 0x00FFFFFF) << 8;

    return (rgb | a);
}

}
