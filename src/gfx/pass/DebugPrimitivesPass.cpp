#include <gfx\pass\DebugPrimitivesPass.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>

#include <common\debug_draw.slang>

namespace GFX
{

PassID DebugPrimitivesPass::GetID() const
{
    return PassID::DebugPrimitives;
}

void DebugPrimitivesPass::Initialize(RenderGraph& graph)
{
}

void DebugPrimitivesPass::RegisterResources(RenderGraph& graph)
{
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DebugDrawBuffer), sizeof(DebugDrawBuffer), VKW::RESOURCE_ACCESS_GENERIC_READ);
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::SortedDebugDrawBuffer), sizeof(DebugDrawBuffer), VKW::RESOURCE_ACCESS_GENERIC_RW);
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DebugDrawCounters), sizeof(DebugDrawCounters), VKW::RESOURCE_ACCESS_GENERIC_RW);
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DebugDrawIndirectArgs), sizeof(DrawIndirectCommand) * uint(DebugDrawCommandType::COUNT), VKW::RESOURCE_ACCESS_INDIRECT_ARGS);
}

void DebugPrimitivesPass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(DebugPrimitivesPass);

    DRE::U32 const renderWidth  = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth;
    DRE::U32 const renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

    StorageBuffer* debugDrawBuffer  = graph.GetBuffer(RESOURCE_ID(BufferID::DebugDrawBuffer));
    StorageBuffer* sortedDrawBuffer = graph.GetBuffer(RESOURCE_ID(BufferID::SortedDebugDrawBuffer));
    StorageBuffer* counters         = graph.GetBuffer(RESOURCE_ID(BufferID::DebugDrawCounters));
    StorageBuffer* indirectArgs     = graph.GetBuffer(RESOURCE_ID(BufferID::DebugDrawIndirectArgs));

    auto& dependencyManager = g_GraphicsManager->GetDependencyManager();

    {
        DRE_GPU_SCOPE(DebugPrimitivesSort);

        dependencyManager.ResourceBarrier(context, counters->GetResource(), VKW::RESOURCE_ACCESS_GENERIC_WRITE, VKW::STAGE_TRANSFER);
        context.CmdFillBuffer(counters->GetResource(), 0, sizeof(DebugDrawCounters), 0);

        dependencyManager.ResourceBarrier(context, counters->GetResource(),         VKW::RESOURCE_ACCESS_GENERIC_RW,   VKW::STAGE_COMPUTE);
        dependencyManager.ResourceBarrier(context, debugDrawBuffer->GetResource(),  VKW::RESOURCE_ACCESS_GENERIC_READ, VKW::STAGE_COMPUTE);
        dependencyManager.ResourceBarrier(context, sortedDrawBuffer->GetResource(), VKW::RESOURCE_ACCESS_GENERIC_RW,   VKW::STAGE_COMPUTE);
        dependencyManager.ResourceBarrier(context, indirectArgs->GetResource(),     VKW::RESOURCE_ACCESS_GENERIC_RW,   VKW::STAGE_COMPUTE);

        DRE::U32 constexpr countGroupCount = DEBUG_DRAW_COMMAND_CAPACITY / 64;

        PipelineEntry* countEntry = g_GraphicsManager->GetPipelineDB().GetEntry("debug_primitives_count");
        ResourceBinder countBinder = g_GraphicsManager->CreateResourceBinder(countEntry, 0);
        countBinder.AddStorageBuffer(0, counters);
        countBinder.AddStorageBuffer(1, debugDrawBuffer);
        countBinder.AddStorageBuffer(2, sortedDrawBuffer);
        countBinder.AddStorageBuffer(3, indirectArgs);
        countBinder.FlushDescriptorWrites();
        context.CmdBindComputeDescriptorSets(countEntry->GetLayout(), countBinder.GetTargetSetID(), 1, &countBinder.GetDescriptorSet());
        context.CmdBindComputePipeline(countEntry->GetPipeline());
        context.CmdDispatch(countGroupCount, 1, 1);

        context.CmdMemoryDependency(
            VKW::RESOURCE_ACCESS_GENERIC_RW, VKW::STAGE_COMPUTE,
            VKW::RESOURCE_ACCESS_GENERIC_RW, VKW::STAGE_COMPUTE);

        PipelineEntry* sortEntry = g_GraphicsManager->GetPipelineDB().GetEntry("debug_primitives_sort");
        ResourceBinder sortBinder = g_GraphicsManager->CreateResourceBinder(sortEntry, 0);
        sortBinder.AddStorageBuffer(0, counters);
        sortBinder.AddStorageBuffer(1, debugDrawBuffer);
        sortBinder.AddStorageBuffer(2, sortedDrawBuffer);
        sortBinder.AddStorageBuffer(3, indirectArgs);
        sortBinder.FlushDescriptorWrites();
        context.CmdBindComputeDescriptorSets(sortEntry->GetLayout(), sortBinder.GetTargetSetID(), 1, &sortBinder.GetDescriptorSet());
        context.CmdBindComputePipeline(sortEntry->GetPipeline());
        context.CmdDispatch(countGroupCount, 1, 1);

        context.CmdMemoryDependency(
            VKW::RESOURCE_ACCESS_GENERIC_RW, VKW::STAGE_COMPUTE,
            VKW::RESOURCE_ACCESS_GENERIC_RW, VKW::STAGE_COMPUTE);

        PipelineEntry* fillArgsEntry = g_GraphicsManager->GetPipelineDB().GetEntry("debug_primitives_fill_args");
        ResourceBinder fillArgsBinder = g_GraphicsManager->CreateResourceBinder(fillArgsEntry, 0);
        fillArgsBinder.AddStorageBuffer(0, counters);
        fillArgsBinder.AddStorageBuffer(1, debugDrawBuffer);
        fillArgsBinder.AddStorageBuffer(2, sortedDrawBuffer);
        fillArgsBinder.AddStorageBuffer(3, indirectArgs);
        fillArgsBinder.FlushDescriptorWrites();
        context.CmdBindComputeDescriptorSets(fillArgsEntry->GetLayout(), fillArgsBinder.GetTargetSetID(), 1, &fillArgsBinder.GetDescriptorSet());
        context.CmdBindComputePipeline(fillArgsEntry->GetPipeline());
        context.CmdDispatch(1, 1, 1);
    }

    {
        DRE_GPU_SCOPE(DebugPrimitivesDraw);

        Texture* output      = graph.GetTexture(RESOURCE_ID(TextureID::DisplayEncodedImage));
        Texture* depthBuffer = graph.GetTexture(RESOURCE_ID(TextureID::MainDepth));

        dependencyManager.ResourceBarrier(context, indirectArgs->GetResource(),     VKW::RESOURCE_ACCESS_INDIRECT_ARGS,           VKW::STAGE_ALL_GRAPHICS);
        dependencyManager.ResourceBarrier(context, sortedDrawBuffer->GetResource(), VKW::RESOURCE_ACCESS_GENERIC_READ,             VKW::STAGE_ALL_GRAPHICS);
        dependencyManager.ResourceBarrier(context, output->GetResource(),           VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT,         VKW::STAGE_COLOR_OUTPUT);
        dependencyManager.ResourceBarrier(context, depthBuffer->GetResource(),      VKW::RESOURCE_ACCESS_DEPTH_STENCIL_ATTACHMENT, VKW::STAGE_ALL_GRAPHICS);

        PipelineEntry* drawEntry = g_GraphicsManager->GetPipelineDB().GetEntry("debug_primitives_draw");
        ResourceBinder drawBinder = g_GraphicsManager->CreateResourceBinder(drawEntry, 0);
        drawBinder.AddStorageBuffer(0, counters);
        drawBinder.AddStorageBuffer(1, debugDrawBuffer);
        drawBinder.AddStorageBuffer(2, sortedDrawBuffer);
        drawBinder.AddStorageBuffer(3, indirectArgs);
        drawBinder.FlushDescriptorWrites();

        context.CmdBindGraphicsDescriptorSets(drawEntry->GetLayout(), drawBinder.GetTargetSetID(), 1, &drawBinder.GetDescriptorSet());
        context.CmdBindGraphicsPipeline(drawEntry->GetPipeline());

        DRE::U32 constexpr attachmentsCount = 1;
        VKW::ImageResourceView* renderTargets[attachmentsCount] = { output->GetShaderView() };

        context.CmdBeginRendering(attachmentsCount, renderTargets, depthBuffer->GetShaderView(), nullptr);
        context.CmdSetViewport(attachmentsCount, 0, 0, renderWidth, renderHeight);
        context.CmdSetScissor(attachmentsCount, 0, 0, renderWidth, renderHeight);
#ifndef DRE_COMPILE_FOR_RENDERDOC
        context.CmdSetPolygonMode(VKW::POLYGON_FILL);
#endif

        for (DRE::U32 i = 0; i < DRE::U32(DebugDrawCommandType::COUNT); ++i)
        {
            context.CmdDrawIndirect(indirectArgs->GetResource(), i * sizeof(DrawIndirectCommand));
        }

        context.CmdEndRendering();
    }
}

}
