#include <editor\TextureInspector.hpp>

#include <foundation\Common.hpp>
#include <foundation\string\InplaceString.hpp>

#include <editor\RootEditor.hpp>
#include <engine\ApplicationContext.hpp>

#include <gfx\GraphicsManager.hpp>

#include <glm\gtc\type_ptr.hpp>

#include <imgui.h>

namespace EDITOR
{

TextureInspector::TextureInspector(BaseEditor* rootEditor, EditorFlags flags, GFX::TextureBank* bank, GFX::GraphResourcesManager* graphResources)
    : BaseEditor{ rootEditor, flags }
    , m_TextureBank{ bank }
    , m_GraphResources{ graphResources }
{}

TextureInspector::TextureInspector(TextureInspector&& rhs)
    : BaseEditor{ DRE_MOVE(rhs) }
    , m_TextureBank{ nullptr }
{
    operator=(DRE_MOVE(rhs));
}

TextureInspector& TextureInspector::operator=(TextureInspector&& rhs)
{
    BaseEditor::operator=(DRE_MOVE(rhs));

    DRE_SWAP_MEMBER(m_TextureBank);
    DRE_SWAP_MEMBER(m_GraphResources);

    return *this;
}

void TextureInspector::Render()
{
    ImGui::SetNextWindowSize(ImVec2(500, 440), ImGuiCond_FirstUseEver);

    bool isOpen = true;
    if (ImGui::Begin("Texture Inspector", &isOpen, ImGuiWindowFlags_None))
    {
        int i = 0;
        if (ImGui::BeginChild("nodes_tree", ImVec2(150, 0), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX))
        {
            if (ImGui::BeginChild("graph_textures"))
            {
                m_GraphResources->ForEachTexture([this](auto& texture)
                {
                    char const* name = texture.value->texture.GetShaderView()->parentResource_->name_.GetData();
                    if (ImGui::Button(name))
                    {
                        m_DisplayedTextures.EmplaceBack(texture.value->texture.GetShaderView());
                    }
                });
            }
            ImGui::EndChild();

            if (ImGui::BeginChild("bank_textures"))
            {
                m_TextureBank->ForEachTexture([](auto& texture)
                    {
                        auto handle = texture.value->GetImGuiDescriptor().GetHandle();
                        ImGui::Image(handle, ImVec2(100,100));
                
                        GFX::g_GraphicsManager->GetImGuiSyncQueue().EmplaceBack(texture.value);
                    });
            }
            ImGui::EndChild();
        };
        ImGui::EndChild();

        ImGui::SameLine();

        if (ImGui::BeginChild("viewer"))
        {
            //if (DRE::g_AppContext.m_FocusedObject != nullptr)
            {
#ifdef DRE_IMGUI_CUSTOM_TEXTURE
                //m_TextureBank->ForEachTexture([](auto& texture)
                //    {
                //        auto handle = texture.value->GetImGuiDescriptor().GetHandle();
                //        ImGui::Image(handle, ImVec2(100,100));
                //
                //        GFX::g_GraphicsManager->GetImGuiSyncQueue().EmplaceBack(texture.value);
                //    });

                m_GraphResources->ForEachTexture([](auto& texture)
                    {
                        auto handle = texture.value->texture.GetImGuiDescriptor().GetHandle();
                        //if(*texture.key == GFX::TextureID::FFTButterfly ||
                        //    *texture.key == GFX::TextureID::ColorHistoryBuffer0||
                        //    *texture.key == GFX::TextureID::FFTPingPong1||
                        //    *texture.key == GFX::TextureID::ColorHistoryBuffer1||
                        //    *texture.key == GFX::TextureID::DisplayEncodedImage)
                        //    return;
                        ImGui::Image(handle, ImVec2(200, 200));

                        GFX::g_GraphicsManager->GetImGuiSyncQueue().EmplaceBack(&texture.value->texture);
                    });
#endif
            }
            //else
            //{
            //    ImGui::TextDisabled("No Node selected");
            //}
        }
        ImGui::EndChild();
    }

    ImGui::End();

    if (!isOpen)
    {
        Close();
    }
}

}
