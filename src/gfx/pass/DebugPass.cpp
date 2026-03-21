#include <gfx\pass\DebugPass.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>
#include <gfx\renderer\DrawBatcher.hpp>

#include <engine\ApplicationContext.hpp>

#include <debug_view.h>

namespace GFX
{

PassID DebugPass::GetID() const
{
    return PassID::Debug;
}

void DebugPass::Initialize(RenderGraph& graph)
{
}

void DebugPass::RegisterResources(RenderGraph& graph)
{
    std::uint32_t renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth, renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

    graph.RegisterTexture(this, RESOURCE_ID(TextureID::DisplayEncodedImage), g_GraphicsManager->GetFinalImageFormat(),
        renderWidth, renderHeight,
        VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE,
        0);

    graph.RegisterUniformBuffer(this, VKW::STAGE_COMPUTE, 1);
}

void DebugPass::Render(RenderGraph& graph, VKW::Context& context)
{
    auto& viewContext = DRE::g_AppContext.m_TextureInspectorViewState;
    if (!viewContext.m_DrawTexture)
    {
        return;
    }

    GFX::Texture* displayedTexture = g_GraphicsManager->GetTextureBank().FindTexture(viewContext.m_TextureName);
    if (displayedTexture == nullptr)
    {
        displayedTexture = graph.GetTexture(viewContext.m_TextureName);

        if (displayedTexture == nullptr)
        {
            return;
        }
    }

    DRE_GPU_SCOPE(DebugPass);

    VKW::ImageResourceView* output = graph.GetTexture(RESOURCE_ID(TextureID::DisplayEncodedImage))->GetShaderView();
    VKW::ImageResourceView* displayedTextureView = displayedTexture->GetShaderView();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, output->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, displayedTextureView->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);


    DebugViewArgs args{};
    args.textureID = displayedTexture->GetShaderGlobalDescriptor().id_;
    args.size = viewContext.m_Size;
    args.lowBound = viewContext.m_LowerEnd;
    args.highBound = viewContext.m_UpperEnd;
    args.channelMask = (viewContext.m_ShowX ? 0x1 : 0) | (viewContext.m_ShowY ? 0x2 : 0) | (viewContext.m_ShowZ ? 0x4 : 0) | (viewContext.m_ShowW ? 0x8 : 0);

    UniformProxy uniform = graph.GetPassUniform(GetID(), context, sizeof(DebugViewArgs));
    uniform.WriteMember140(args);

    VKW::DescriptorSet set = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());

    VKW::PipelineLayout* layout = graph.GetPassPipelineLayout(GetID());
    VKW::Pipeline* pipeline = g_GraphicsManager->GetPipelineDB().GetPipeline("debug_view");

    context.CmdBindComputeDescriptorSets(pipeline->GetLayout(), graph.GetPassSetBinding(), 1, &set);
    context.CmdBindComputePipeline(pipeline);

    glm::uvec2 imageSize{ g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth, g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight };
    glm::uvec2 const groupSize{ 8, 8 };
    glm::uvec2 const dispatchSize = imageSize / groupSize + 1u;

    context.CmdDispatch(dispatchSize.x * 4, dispatchSize.y, 1);

    //for (std::uint32_t i = 0; i < 4; i++)
    //{
    //    float texture_offset[2] = { static_cast<float>(i) + 0.5, 256.0f * i };
    //    //context.CmdPushConstants(layout, VKW::DESCRIPTOR_STAGE_COMPUTE, 0, 8, &texture_offset);
    //    context.CmdDispatch(dispatchSize.x, dispatchSize.y, 1);
    //}
    
}

}
