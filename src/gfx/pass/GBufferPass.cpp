#include <gfx\pass\GBufferPass.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>
#include <gfx\renderer\DrawBatcher.hpp>

#include <engine\ApplicationContext.hpp>


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
        RESOURCE_ID(TextureID::GBufferC_Velocity),
        gBufferFormats[2], renderWidth, renderHeight,
        2);

    graph.RegisterRenderTarget(this,
        RESOURCE_ID(TextureID::GBufferD_ObjectIDBuffer),
        gBufferFormats[3], renderWidth, renderHeight,
        3);

    graph.RegisterRenderTarget(this,
        RESOURCE_ID(TextureID::DEBUG_TEXTURE),
        VKW::FORMAT_R32G32B32A32_FLOAT, renderWidth, renderHeight,
        4);

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

    DRE::U32 renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth;
    DRE::U32 renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

    Texture* attachmentA = graph.GetTexture(RESOURCE_ID(TextureID::GBufferA_DiffuseRoughness));
    Texture* attachmentB = graph.GetTexture(RESOURCE_ID(TextureID::GBufferB_NormalMetalness));
    Texture* attachmentC = graph.GetTexture(RESOURCE_ID(TextureID::GBufferC_Velocity));
    Texture* attachmentD = graph.GetTexture(RESOURCE_ID(TextureID::GBufferD_ObjectIDBuffer));
    Texture* attachmentDEBUG = graph.GetTexture(RESOURCE_ID(TextureID::DEBUG_TEXTURE));
    Texture* depthAttachment = graph.GetTexture(RESOURCE_ID(TextureID::MainDepth));

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, attachmentA->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, attachmentB->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, attachmentC->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, attachmentD->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, attachmentDEBUG->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, depthAttachment->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_DEPTH_ONLY_ATTACHMENT, VKW::STAGE_ALL_GRAPHICS);

    DRE::U32 constexpr attachmentsCount = 5;
    VKW::ImageResourceView* attachments[attachmentsCount] = {
        attachmentA->GetShaderView(),
        attachmentB->GetShaderView(),
        attachmentC->GetShaderView(),
        attachmentD->GetShaderView(),
        attachmentDEBUG->GetShaderView()
    };

    PipelineEntry* entry = g_GraphicsManager->GetPipelineDB().GetEntry("gbuffer_pbr");

    DrawBatcher batcher{ &DRE::g_FrameScratchAllocator, g_GraphicsManager->GetMainDevice()->GetDescriptorManager(), &g_GraphicsManager->GetUniformArena() };

    // WARNING!!!!!!!!!!!!!!!
    batcher.Batch(context, g_GraphicsManager->GetMainRenderView(), entry->GetLayout(), RenderableObject::LAYER_GBUFFER, nullptr/*GFX::ForwardObjectDelegate*/);

    context.CmdBeginRendering(attachmentsCount, attachments, depthAttachment->GetShaderView(), nullptr);
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context.CmdClearAttachments(VKW::ATTACHMENT_MASK_COLOR_0 | VKW::ATTACHMENT_MASK_COLOR_1 |
                                 VKW::ATTACHMENT_MASK_COLOR_2 | VKW::ATTACHMENT_MASK_COLOR_3, clearColor);
    context.CmdClearAttachments(VKW::ATTACHMENT_MASK_DEPTH, 0.0f, 0);

    context.CmdSetViewport(attachmentsCount, 0, 0, renderWidth, renderHeight);
    context.CmdSetScissor(attachmentsCount, 0, 0, renderWidth, renderHeight);
#ifndef DRE_COMPILE_FOR_RENDERDOC
    context.CmdSetPolygonMode(VKW::POLYGON_FILL);
#endif // DRE_COMPILE_FOR_RENDERDOC


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

        context.CmdBindVertexBuffer(atom.vertexBuffer, atom.vertexOffset);
        context.CmdBindIndexBuffer(atom.indexBuffer, atom.indexOffset);
        context.CmdPushConstants(entry->GetLayout(), VKW::DESCRIPTOR_STAGE_ALL, 0, sizeof(DRE::U32), &atom.instanceID);
        context.CmdDrawIndexed(atom.indexCount);
    }

    context.CmdEndRendering();

    ReadbackScheduler readback(g_GraphicsManager->GetCurrentFrameID(), &g_GraphicsManager->GetReadbackArena(), renderWidth * renderHeight * 4);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, attachmentD->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_TRANSFER_SRC, VKW::STAGE_TRANSFER);
    context.CmdCopyImageToBuffer(readback.GetDstBuffer(), attachmentD->GetShaderView()->parentResource_, readback.GetDstOffset());

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
