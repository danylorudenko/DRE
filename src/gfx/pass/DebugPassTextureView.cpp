#include <gfx\pass\DebugPassTextureView.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>

#include <engine\ApplicationContext.hpp>

#include <debug_view.slang>

namespace GFX
{

PassID DebugPassTextureView::GetID() const
{
    return PassID::DebugTextureView;
}

void DebugPassTextureView::Initialize(RenderGraph& graph)
{
}

void DebugPassTextureView::RegisterResources(RenderGraph& graph)
{
    DRE::U32 renderWidth  = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth;
    DRE::U32 renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

    graph.RegisterTexture(this, RESOURCE_ID(TextureID::DisplayEncodedImage), g_GraphicsManager->GetFinalImageFormat(),
        renderWidth, renderHeight,
        VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE,
        0);

    graph.RegisterUniformBuffer(this, VKW::STAGE_COMPUTE, 1);
}

void DebugPassTextureView::Render(RenderGraph& graph, VKW::Context& context)
{
    auto& viewContext = DRE::g_AppContext.m_TextureInspectorViewState;
    if (!viewContext.m_DrawTexture)
        return;

    GFX::Texture* displayedTexture = g_GraphicsManager->GetTextureBank().FindTexture(viewContext.m_TextureName);
    if (displayedTexture == nullptr)
    {
        displayedTexture = graph.GetTexture(viewContext.m_TextureName);
        if (displayedTexture == nullptr)
            return;
    }

    DRE_GPU_SCOPE(DebugPassTextureView);

    glm::uvec2 outputImageSize{ g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth, g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight };

    VKW::ImageResourceView* output               = graph.GetTexture(RESOURCE_ID(TextureID::DisplayEncodedImage))->GetShaderView();
    VKW::ImageResourceView* displayedTextureView = displayedTexture->GetShaderView();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, output->parentResource_,               VKW::RESOURCE_ACCESS_SHADER_WRITE,  VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, displayedTextureView->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);

    DebugViewArgs args{};
    args.textureID   = displayedTexture->GetShaderGlobalDescriptor().id_;
    args.size        = viewContext.m_Size;
    args.lowBound    = viewContext.m_LowerEnd;
    args.highBound   = viewContext.m_UpperEnd;
    args.channelMask = (viewContext.m_ShowX ? 0x1 : 0) | (viewContext.m_ShowY ? 0x2 : 0) | (viewContext.m_ShowZ ? 0x4 : 0) | (viewContext.m_ShowW ? 0x8 : 0);

    UniformProxy uniform = graph.GetPassUniform(GetID(), context, sizeof(DebugViewArgs));
    uniform.WriteMember140(args);

    VKW::DescriptorSet   set      = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());
    VKW::PipelineLayout* layout   = graph.GetPassPipelineLayout(GetID());
    VKW::Pipeline*       pipeline = g_GraphicsManager->GetPipelineDB().GetPipeline("debug_view_texture");

    context.CmdBindComputeDescriptorSets(pipeline->GetLayout(), graph.GetPassSetBinding(), 1, &set);
    context.CmdBindComputePipeline(pipeline);

    glm::uvec3 const textureViewerGroupSize{ 8, 8, 1 };
    glm::uvec3 const dispatchSize = GetComputeGroupCount(glm::uvec3(outputImageSize, 1), textureViewerGroupSize);
    context.CmdDispatch(dispatchSize.x, dispatchSize.y, dispatchSize.z);
}

}
