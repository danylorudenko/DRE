#include <gfx\pass\GBufferPass.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>
#include <gfx\renderer\DrawBatcher.hpp>

#include <engine\ApplicationContext.hpp>
#include <engine\io\IOManager.hpp>
#include <engine\scene\Scene.hpp>


namespace GFX
{

PassID GBufferPass::GetID() const
{
    return PassID::GBuffer;
}

void GBufferPass::RegisterResources(RenderGraph& graph)
{
    std::uint32_t renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth,
        renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

    auto gBufferFormats = g_GraphicsManager->GetGBufferFormats();

    graph.RegisterRenderTarget(this,
        RESOURCE_ID(TextureID::GBufferA_DiffuseRoughness),
        gBufferFormats[0], renderWidth, renderHeight,
        0);

    graph.RegisterRenderTarget(this,
        RESOURCE_ID(TextureID::GBufferB_NormalMetalness),
        gBufferFormats[1], renderWidth, renderHeight,
        1);

    graph.RegisterRenderTarget(this,
        RESOURCE_ID(TextureID::Velocity),
        gBufferFormats[2], renderWidth, renderHeight,
        2);

    graph.RegisterRenderTarget(this,
        RESOURCE_ID(TextureID::ObjectIDBuffer),
        gBufferFormats[3], renderWidth, renderHeight,
        3);

    graph.RegisterDepthOnlyTarget(this,
        RESOURCE_ID(TextureID::MainDepth),
        g_GraphicsManager->GetMainDepthFormat(), renderWidth, renderHeight);
}

void GBufferPass::Initialize(RenderGraph& graph)
{
}

void GBufferPass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(GBuffer);

    std::uint32_t renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth,
        renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

    VKW::ImageResourceView* attachmentA = graph.GetTexture(RESOURCE_ID(TextureID::GBufferA_DiffuseRoughness))->GetShaderView();
    VKW::ImageResourceView* attachmentB = graph.GetTexture(RESOURCE_ID(TextureID::GBufferB_NormalMetalness))->GetShaderView();
    VKW::ImageResourceView* attachmentC = graph.GetTexture(RESOURCE_ID(TextureID::Velocity))->GetShaderView();
    VKW::ImageResourceView* attachmentD = graph.GetTexture(RESOURCE_ID(TextureID::ObjectIDBuffer))->GetShaderView();
    VKW::ImageResourceView* depthAttachment = graph.GetTexture(RESOURCE_ID(TextureID::MainDepth))->GetShaderView();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, attachmentA->parentResource_, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, attachmentB->parentResource_, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, attachmentC->parentResource_, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, attachmentD->parentResource_, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, depthAttachment->parentResource_, VKW::RESOURCE_ACCESS_DEPTH_ONLY_ATTACHMENT, VKW::STAGE_ALL_GRAPHICS);

    std::uint32_t constexpr attachmentsCount = 4;
    VKW::ImageResourceView* attachments[attachmentsCount] = { attachmentA, attachmentB, attachmentC, attachmentD };

    //VKW::PipelineLayout* passLayout = graph.GetPassPipelineLayout(GetID());
    VKW::PipelineLayout* passLayout = g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->GetGlobalPipelineLayout();
    DrawBatcher batcher{ &DRE::g_FrameScratchAllocator, g_GraphicsManager->GetMainDevice()->GetDescriptorManager(), &g_GraphicsManager->GetUniformArena() };

    // WARNING!!!!!!!!!!!!!!!
    batcher.Batch(context, g_GraphicsManager->GetMainRenderView(), passLayout, RenderableObject::LAYER_GBUFFER, nullptr/*GFX::ForwardObjectDelegate*/);

    context.CmdBeginRendering(attachmentsCount, attachments, depthAttachment, nullptr);
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context.CmdClearAttachments(VKW::ATTACHMENT_MASK_COLOR_0 | VKW::ATTACHMENT_MASK_COLOR_1 |
                                 VKW::ATTACHMENT_MASK_COLOR_2 | VKW::ATTACHMENT_MASK_COLOR_3, clearColor);
    context.CmdClearAttachments(VKW::ATTACHMENT_MASK_DEPTH, 0.0f, 0);

    context.CmdSetViewport(attachmentsCount, 0, 0, renderWidth, renderHeight);
    context.CmdSetScissor(attachmentsCount, 0, 0, renderWidth, renderHeight);
#ifndef DRE_COMPILE_FOR_RENDERDOC
    context.CmdSetPolygonMode(VKW::POLYGON_FILL);
#endif // DRE_COMPILE_FOR_RENDERDOC



    //VKW::DescriptorSet passSet = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());
    //context.CmdBindDescriptorSets(passLayout, VKW::BindPoint::Graphics, graph.GetPassSetBinding(), 1, &passSet);

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

        // push constant here?
        //context.CmdBindGraphicsDescriptorSets(atom.pipeline->GetLayout(), userSetBinding, 1, &atom.descriptorSet);
        context.CmdBindVertexBuffer(atom.vertexBuffer, atom.vertexOffset);
        context.CmdBindIndexBuffer(atom.indexBuffer, atom.indexOffset);
        context.CmdPushConstants(passLayout, VKW::DESCRIPTOR_STAGE_ALL, 0, sizeof(std::uint32_t), &atom.instanceID);
        context.CmdDrawIndexed(atom.indexCount);
    }

    context.CmdEndRendering();

    ReadbackScheduler readback(g_GraphicsManager->GetCurrentFrameID(), &g_GraphicsManager->GetReadbackArena(), renderWidth * renderHeight * 4);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, attachmentD->parentResource_, VKW::RESOURCE_ACCESS_TRANSFER_SRC, VKW::STAGE_TRANSFER);
    context.CmdCopyImageToBuffer(readback.GetDstBuffer(), attachmentD->parentResource_, readback.GetDstOffset());

    ReadbackFuture tempFuture = readback.CreateReadbackFuture(context.SyncPoint());

    if (static_cast<bool>(m_LastObjectIDsFuture))
    {
        m_LastObjectIDsFuture.Sync();
        void* readbackData = m_LastObjectIDsFuture.GetMappedPtr();

        DRE::S32 x = DRE::Clamp(DRE::g_AppContext.m_CursorX, 0, DRE::S32(renderWidth - 1));
        DRE::S32 y = DRE::Clamp(DRE::g_AppContext.m_CursorY, 0, DRE::S32(renderHeight - 1));
        DRE::g_AppContext.m_MouseHoveredObjectID = ObjectIDFromBuffer(readbackData, x, y);
    }

    m_LastObjectIDsFuture = tempFuture;
    context.CmdBindGlobalDescriptorSets(*g_GraphicsManager->GetMainDevice()->GetDescriptorManager(), g_GraphicsManager->GetCurrentFrameID());
}

DRE::U32 GBufferPass::ObjectIDFromBuffer(void* ptr, DRE::U32 x, DRE::U32 y)
{
    DRE::U32 const xOffset = x * 4;
    DRE::U32 const yOffset = y * 4 * g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth;

    DRE::U32 const pixel = *(DRE::U32 const*)((DRE::U8 const*)ptr + (xOffset + yOffset));

    DRE::U32 const a = (pixel & 0xFF000000) >> 24;
    DRE::U32 const rgb = (pixel & 0x00FFFFFF) << 8;

    return (rgb | a);
}

}
