#include <gfx\pass\DebugPass.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>
#include <gfx\renderer\DDGI.hpp>

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

    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DebugPassDDGIProbeIndirectArgs), sizeof(DrawIndirectCommand), VKW::RESOURCE_ACCESS_INDIRECT_ARGS, VKW::STAGE_ALL_GLOBAL, 2);
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DDGI_ProbeData), DDGI::GetProbeDataBufferSize(), VKW::RESOURCE_ACCESS_GENERIC_READ, VKW::STAGE_ALL_GLOBAL, 3);
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

    glm::uvec2 outputImageSize{ g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth, g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight };

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

    glm::uvec3 const textureViewerGroupSize{ 8, 8, 1 };
    glm::uvec3 const dispatchSize = GetComputeGroupCount(glm::uvec3(outputImageSize, 1), textureViewerGroupSize);
    context.CmdDispatch(dispatchSize.x, dispatchSize.y, dispatchSize.z);


    // DDGI PROBES DEBUG
    VKW::BufferResource* ddgiProbeIndirectArgs = graph.GetBuffer(RESOURCE_ID(BufferID::DebugPassDDGIProbeIndirectArgs))->GetResource();
    VKW::BufferResource* ddgiProbeData = graph.GetBuffer(RESOURCE_ID(BufferID::DDGI_ProbeData))->GetResource();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeIndirectArgs, VKW::RESOURCE_ACCESS_GENERIC_WRITE, VKW::STAGE_TRANSFER);
    context.CmdFillBuffer(ddgiProbeIndirectArgs, 0, sizeof(DrawIndirectCommand), 0);


    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeIndirectArgs, VKW::RESOURCE_ACCESS_GENERIC_RW, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, ddgiProbeData, VKW::RESOURCE_ACCESS_GENERIC_READ, VKW::STAGE_COMPUTE);
    glm::uvec3 const probeDebugGroupSize{ 4, 4, 4 };
    glm::uvec3 const probeDebugDisplatchSize = GetComputeGroupCount(DDGI::GetProbeCount3D(), probeDebugGroupSize);
    context.CmdDispatch(probeDebugDisplatchSize.x, probeDebugDisplatchSize.y, probeDebugDisplatchSize.z);
}

}
