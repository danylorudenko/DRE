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
    graph.RegisterRenderTarget(this,
        TextureID::FinalRT,
        VKW::FORMAT_B8G8R8A8_UNORM, g_GraphicsManager->GetRenderingWidth(), g_GraphicsManager->GetRenderingHeight(),
        0);
}

void ForwardOpaquePass::Initialize(RenderGraph& graph)
{
    //std::uint64_t constexpr LINEAR_MEMORY_SIZE = 64 * 1024;
    //
    //DRE::ByteBuffer vertexBlob{};
    //IO::IOManager::ReadFileToBuffer("shaders\\test-triangle-nodata.vert.spv", vertexBlob);
    //VKW::ShaderModule vertexModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), vertexBlob, VKW::SHADER_MODULE_TYPE_VERTEX, "main"};
    //
    //DRE::ByteBuffer fragmentBlob{};
    //IO::IOManager::ReadFileToBuffer("shaders\\test-triangle-nodata.frag.spv", fragmentBlob);
    //VKW::ShaderModule fragmentModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), fragmentBlob, VKW::SHADER_MODULE_TYPE_FRAGMENT, "main"};
    //
    /////////////////////
    //
    //VKW::PipelineLayout* layout = graph.GetPassPipelineLayout(GetID());
    //
    //VKW::Pipeline::Descriptor pipeDesc;
    //pipeDesc.SetPipelineType(VKW::PIPELINE_TYPE_GRAPHIC);
    //pipeDesc.SetLayout(layout);
    //pipeDesc.SetVertexShader(vertexModule);
    //pipeDesc.SetFragmentShader(fragmentModule);
    //pipeDesc.AddOutputViewport(VKW::FORMAT_B8G8R8A8_UNORM, VKW::BLEND_TYPE_NONE);
    //g_GraphicsManager->GetPipelineDB().CreatePipeline("triangle_pipeline", pipeDesc);

}

void ForwardOpaquePass::Render(RenderGraph& graph, VKW::Context& context)
{
    VKW::ImageResourceView* colorAttachment = graph.GetStorageTexture(TextureID::FinalRT)->GetShaderView();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, colorAttachment->parentResource_, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    context.CmdBeginRendering(1, colorAttachment, nullptr, nullptr);
    float clearColors[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context.CmdClearAttachments(VKW::ATTACHMENT_MASK_COLOR_0, clearColors);

    context.CmdSetViewport(1, 0, 0, g_GraphicsManager->GetRenderingWidth(), g_GraphicsManager->GetRenderingHeight());
    context.CmdSetScissor(1, 0, 0, g_GraphicsManager->GetRenderingWidth(), g_GraphicsManager->GetRenderingHeight());

    // 1. take all RenderableObject's in main scene
    DrawBatcher batcher{ &DRE::g_FrameScratchAllocator, g_GraphicsManager->GetMainDevice()->GetDescriptorManager(), &g_GraphicsManager->GetUniformArena() };
    auto& allEntities = WORLD::g_MainScene->GetEntities();
    for (std::uint32_t i = 0, size = allEntities.Size(); i < size; i++)
    {
        batcher.AddRenderable(allEntities[i].GetRenderableObject());
    }

    batcher.Batch(context, *WORLD::g_MainScene);

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
        context.CmdDraw(atom.indexCount);
    }

    // 2. feed all RenderableObject's to DrawBatcher
    // 3. DrawBatcher produces atomic draw commands
    context.CmdEndRendering();
}

}
