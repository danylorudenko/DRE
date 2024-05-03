#include <gfx\pass\ImGuiRenderPass.hpp>

#include <foundation\memory\ByteBuffer.hpp>

#include <vk_wrapper\pipeline\ShaderModule.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>
#include <gfx\pipeline\PipelineDB.hpp>

#include <imgui.h>
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

    graph.RegisterUniformBuffer(this, VKW::STAGE_VERTEX | VKW::STAGE_FRAGMENT, 0);
}

void ImGuiRenderPass::Initialize(RenderGraph& graph)
{
    //ImGuiIO& io = ImGui::GetIO();

    //unsigned char* textureData = nullptr;
    //int imguiAtlasWidth = 0, imguiAtlasHeight = 0, imguiPixelBytes = 0;
    //io.Fonts->GetTexDataAsAlpha8(&textureData, &imguiAtlasWidth, &imguiAtlasHeight, &imguiPixelBytes);
    //
    //DRE::ByteBuffer textureBuffer{ textureData, std::uint64_t(imguiAtlasWidth * imguiAtlasHeight * imguiPixelBytes) };
    //m_ImGuiAtlas = g_GraphicsManager->GetTextureBank().LoadTexture2DSync("imgui_atlas", imguiAtlasWidth, imguiAtlasHeight, VKW::FORMAT_R8_UNORM, textureBuffer);

    PipelineDB& pipelineDB = g_GraphicsManager->GetPipelineDB();
    VKW::PipelineLayout* layout = graph.GetPassPipelineLayout(GetID());

    VKW::Pipeline::Descriptor pipelineDescriptor;
    pipelineDescriptor.SetPipelineType(VKW::PIPELINE_TYPE_GRAPHIC);
    pipelineDescriptor.SetLayout(layout);

    pipelineDescriptor.AddVertexAttribute(VKW::FORMAT_R32G32_FLOAT);
    pipelineDescriptor.AddVertexAttribute(VKW::FORMAT_R32G32_FLOAT);
    pipelineDescriptor.AddVertexAttribute(VKW::FORMAT_R8G8B8A8_UNORM);
    pipelineDescriptor.SetWindingOrder(VKW::WINDING_ORDER_CLOCKWIZE);

    DRE::ByteBuffer vertexModuleBuffer;
    IO::IOManager::ReadFileToBuffer("shaders\\imgui.vert.spv", &vertexModuleBuffer);
    VKW::ShaderModule vertexModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), vertexModuleBuffer, VKW::SHADER_MODULE_TYPE_VERTEX, "main"};

    DRE::ByteBuffer fragmentModuleBuffer;
    IO::IOManager::ReadFileToBuffer("shaders\\imgui.frag.spv", &fragmentModuleBuffer);
    VKW::ShaderModule fragmentModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), fragmentModuleBuffer, VKW::SHADER_MODULE_TYPE_FRAGMENT, "main" };

    pipelineDescriptor.SetVertexShader(vertexModule);
    pipelineDescriptor.SetFragmentShader(fragmentModule);
    pipelineDescriptor.SetCullMode(VK_CULL_MODE_NONE);

    pipelineDescriptor.AddColorOutput(VKW::FORMAT_B8G8R8A8_UNORM, VKW::BLEND_TYPE_ALPHA_OVER);

    pipelineDB.CreatePipeline("imgui", pipelineDescriptor);
}

ImGuiRenderPass::~ImGuiRenderPass()
{
}

/////////////////////////
void ImGuiRenderPass::Render(RenderGraph& graph, VKW::Context& context)
{
    //ImGui::Render();
    //ImDrawData* data = ImGui::GetDrawData();

    //ImVec2 imDisplayPos = ImVec2(0, 0);
    //ImVec2 imDisplaySize = ImVec2(GFX::g_GraphicsManager->GetMainWindow()->Width(), GFX::g_GraphicsManager->GetMainWindow()->Height());
    //
    //glm::vec4 displayPos_Size{ imDisplayPos.x, imDisplayPos.y, imDisplaySize.x, imDisplaySize.y };
    //std::uint32_t texID{ m_ImGuiAtlas->GetShaderReadDescriptor().id_ };
    //
    //UniformProxy uniform = graph.GetPassUniform(GetID(), context, 36);
    //
    //uniform.WriteMember140(glm::value_ptr(displayPos_Size), sizeof(displayPos_Size));
    //uniform.WriteMember140(texID);

    VKW::ImageResourceView* imGuiRT = graph.GetTexture(TextureID::DisplayEncodedImage)->GetShaderView();

    //PipelineDB& pipelineDB = g_GraphicsManager->GetPipelineDB();
    //VKW::PipelineLayout* layout = graph.GetPassPipelineLayout(GetID());
    //std::uint16_t const startSet = pipelineDB.GetGlobalLayout()->GetMemberCount();
    //VKW::DescriptorSet passSet = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, imGuiRT->parentResource_, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);

    std::uint32_t renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth, renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

    context.CmdBeginRendering(1, &imGuiRT, nullptr, nullptr);
    context.CmdSetViewport(2, 0, 0, renderWidth, renderHeight);
    context.CmdSetScissor(2, 0, 0, renderWidth, renderHeight);
    //context.CmdBindGraphicsDescriptorSets(layout, startSet, 1, &passSet);
    RenderDREImGui();


    /*
    context.CmdBindPipeline(VKW::BindPoint::Graphics, pipelineDB.GetPipeline("imgui"));
#ifndef DRE_COMPILE_FOR_RENDERDOC
    context.CmdSetPolygonMode(VKW::POLYGON_FILL);
#endif // DRE_COMPILE_FOR_RENDERDOC



    UploadArena& uploadArena =  g_GraphicsManager->GetUploadArena();

    UploadArena::Allocation vertexMemory = uploadArena.AllocateTransientRegion(g_GraphicsManager->GetCurrentFrameID(), data->TotalVtxCount * sizeof(ImDrawVert), 16);
    UploadArena::Allocation indexMemory = uploadArena.AllocateTransientRegion(g_GraphicsManager->GetCurrentFrameID(), data->TotalIdxCount * sizeof(ImDrawIdx), 4);

    std::uint32_t vertexBindingOffset = vertexMemory.m_OffsetInBuffer;
    std::uint32_t indexBindingOffset = indexMemory.m_OffsetInBuffer;

    ImDrawVert* vertexPtr = reinterpret_cast<ImDrawVert*>(vertexMemory.m_MappedRange);
    ImDrawIdx* indexPtr = reinterpret_cast<ImDrawIdx*>(indexMemory.m_MappedRange);


    for (int i = 0; i < data->CmdListsCount; ++i) {
        ImDrawList const* drawList = data->CmdLists[i];
        ImVector<ImDrawVert> const& vertexBuffer = drawList->VtxBuffer;
        ImVector<ImDrawIdx> const& indexBuffer = drawList->IdxBuffer;

        std::memcpy(vertexPtr, vertexBuffer.Data, vertexBuffer.size_in_bytes());
        vertexPtr += vertexBuffer.Size;

        std::memcpy(indexPtr, indexBuffer.Data, indexBuffer.size_in_bytes());
        indexPtr += indexBuffer.Size;

        context.CmdBindVertexBuffer(vertexMemory.m_Buffer, vertexBindingOffset);
        context.CmdBindIndexBuffer(indexMemory.m_Buffer, indexBindingOffset, sizeof(ImDrawIdx) * 8);

//#define DRE_IMGUI_ATOMIC_DRAWS
#if !defined(DRE_IMGUI_ATOMIC_DRAWS)
        for (int j = 0; j < drawList->CmdBuffer.size(); j++)
        {
            ImDrawCmd const& cmd = drawList->CmdBuffer[j];
            context.CmdDrawIndexed(cmd.ElemCount, 1, cmd.IdxOffset, cmd.VtxOffset);
        }
#else
        for (int j = 0; j < drawList->CmdBuffer.size(); j++)
        {
            ImDrawCmd const& cmd = drawList->CmdBuffer[j];
            unsigned int const drawCount = cmd.ElemCount / 3;
            unsigned int drawIdxOffset = cmd.IdxOffset;

            for (int k = 0; k < drawCount; k++)
            {
                context.CmdDrawIndexed(3, 1, drawIdxOffset, cmd.VtxOffset);
                drawIdxOffset += 3;
            }
        }
#endif

        vertexBindingOffset += vertexBuffer.size_in_bytes();
        indexBindingOffset += indexBuffer.size_in_bytes();
    }

    vertexMemory.FlushCaches();
    indexMemory.FlushCaches();
    */

    context.CmdEndRendering();
}


}

