#include <gfx\pass\ImGuiRenderPass.hpp>

#include <foundation\memory\ByteBuffer.hpp>

#include <vk_wrapper\pipeline\ShaderModule.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>
#include <gfx\pipeline\PipelineDB.hpp>

#include <imgui.h>
#include <backends\imgui_impl_vulkan.h>
#include <glm\gtc\type_ptr.hpp>

#include <engine\io\IOManager.hpp>

#include <gfx\GraphicsManager.hpp>

extern void RenderDREImGui();

namespace GFX
{

ImGuiRenderPass::ImGuiRenderPass()
    : BasePass()
    , m_ImGuiAtlas{ nullptr }
    , m_GraphicsPipeline{ nullptr }
{
}

PassID ImGuiRenderPass::GetID() const
{
    return PassID::ImGuiRender;
}

/////////////////////////
void ImGuiRenderPass::RegisterResources(RenderGraph& graph)
{
    graph.RegisterRenderTarget(this, TextureID::DisplayEncodedImage, VKW::FORMAT_B8G8R8A8_UNORM,
        g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth, g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight,
        0);
}

void ImGuiRenderPass::Initialize(RenderGraph& graph)
{
}

ImGuiRenderPass::~ImGuiRenderPass()
{
}

/////////////////////////
void ImGuiRenderPass::Render(RenderGraph& graph, VKW::Context& context)
{
    VKW::ImageResourceView* imGuiRT = graph.GetTexture(TextureID::DisplayEncodedImage)->GetShaderView();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, imGuiRT->parentResource_, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);

    std::uint32_t renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth, renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

    context.CmdBeginRendering(1, &imGuiRT, nullptr, nullptr);
    context.CmdSetViewport(1, 0, 0, renderWidth, renderHeight);
    context.CmdSetScissor(1, 0, 0, renderWidth, renderHeight);

    ImGui::Render();
    ImDrawData* data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(data, *context.GetCurrentCommandList());

    context.CmdEndRendering();

    ImGui::RenderPlatformWindowsDefault();

}


}

