#include <gfx\pass\DebugPass.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>
#include <gfx\renderer\DrawBatcher.hpp>

#include <engine\scene\Scene.hpp>

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

    graph.RegisterTexture(this, RESOURCE_ID(TextureID::FFTH0), VKW::FORMAT_R32G32B32A32_FLOAT, C_WATER_DIM, C_WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE, 1);
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::FFTHxt), VKW::FORMAT_R32G32_FLOAT, C_WATER_DIM, C_WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE, 2);
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::WaterHeight), VKW::FORMAT_R32_FLOAT, C_WATER_DIM, C_WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE, 3);
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::FFTPingPong0), VKW::FORMAT_R32G32_FLOAT, C_WATER_DIM, C_WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE, 4);

    //graph.RegisterPushConstant(this, 8, VKW::STAGE_COMPUTE);
}

void DebugPass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(DebugPass);

    VKW::ImageResourceView* output = graph.GetTexture(RESOURCE_ID(TextureID::DisplayEncodedImage))->GetShaderView();

    VKW::ImageResourceView* fftH0 = graph.GetTexture(RESOURCE_ID(TextureID::FFTH0))->GetShaderView();
    VKW::ImageResourceView* fftHxt = graph.GetTexture(RESOURCE_ID(TextureID::FFTHxt))->GetShaderView();
    VKW::ImageResourceView* height = graph.GetTexture(RESOURCE_ID(TextureID::WaterHeight))->GetShaderView();
    VKW::ImageResourceView* pingPong = graph.GetTexture(RESOURCE_ID(TextureID::FFTPingPong0))->GetShaderView();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, output->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, fftH0->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, fftHxt->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, height->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, pingPong->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_COMPUTE);

    VKW::DescriptorSet set = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());

    VKW::PipelineLayout* layout = graph.GetPassPipelineLayout(GetID());
    VKW::Pipeline* pipeline = g_GraphicsManager->GetPipelineDB().GetPipeline("debug_view");

    context.CmdBindComputeDescriptorSets(pipeline->GetLayout(), graph.GetPassSetBinding(), 1, &set);
    context.CmdBindComputePipeline(pipeline);

    glm::uvec2 imageSize{ C_WATER_DIM, C_WATER_DIM };
    glm::uvec2 const groupSize{ 8, 8 };
    glm::uvec2 const dispatchSize = imageSize / groupSize;

    context.CmdDispatch(dispatchSize.x * 4, dispatchSize.y, 1);

    //for (std::uint32_t i = 0; i < 4; i++)
    //{
    //    float texture_offset[2] = { static_cast<float>(i) + 0.5, 256.0f * i };
    //    //context.CmdPushConstants(layout, VKW::DESCRIPTOR_STAGE_COMPUTE, 0, 8, &texture_offset);
    //    context.CmdDispatch(dispatchSize.x, dispatchSize.y, 1);
    //}
    
}

}
