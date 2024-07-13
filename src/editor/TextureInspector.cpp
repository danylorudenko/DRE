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
        ////////////////////////////
        // Texture List BEGIN
        ////////////////////////////
        if (ImGui::BeginChild("nodes_tree", ImVec2(300, 0), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX))
        {
            ////////////
            // Search Buff
            char searchBuff[32];
            DRE::MemZero(searchBuff, 32);
            ImGui::InputText("Search", searchBuff, 32);
            std::uint32_t searchLength = std::strlen(searchBuff);

            auto TextureInList = [this, &searchBuff, searchLength](GFX::Texture& texture)
            {
                char const* name = texture.GetShaderView()->parentResource_->name_.GetData();
                if (searchLength == 0 || std::strstr(name, searchBuff) != nullptr)
                {
                    if (ImGui::Button(name))
                    {
                        if (m_DisplayedTextures.Size() < m_DisplayedTextures.Capacity())
                        {
                            m_DisplayedTextures.EmplaceBackUnique(&texture);
                        }
                        else
                        {
                            ImGui::OpenPopup("many_views_popup");
                        }
                    }
                }
            };

            ImGui::SeparatorText("Graph Textures");
            {
                m_GraphResources->ForEachTexture([&TextureInList](auto& texture)
                {
                    TextureInList(texture.value->texture);
                });
            }

            ImGui::SeparatorText("Disc Textures");
            {
                m_TextureBank->ForEachTexture([&TextureInList](auto& texture)
                {
                    TextureInList(*texture.value);
                });
            }

            if (ImGui::BeginPopup("many_views_popup"))
            {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Too many textures in the view. Limit is 12");
                ImGui::EndPopup();
            }
        };
        ImGui::EndChild();
        ////////////////////////////
        // Texture List END
        ////////////////////////////


        ImGui::SameLine();


        ////////////////////////////
        // Texture View BEGIN
        ////////////////////////////
        if (ImGui::BeginChild("viewer"))
        {
#ifdef DRE_IMGUI_CUSTOM_TEXTURE

            DRE::InplaceVector<GFX::Texture*, 2> deleteQueue;
            for(std::uint32_t i = 0, size = m_DisplayedTextures.Size(); i < size; i++)
            {
                ////////////////////////////
                // Texture View BEGIN
                ////////////////////////////
                ImVec2 imageSize{ float(m_DisplayedTextures[i]->GetWidth()), float(m_DisplayedTextures[i]->GetHeight()) };
                if (DRE::Max(imageSize.x, imageSize.y) > 512)
                {
                    imageSize.x *= 512.0f / DRE::Max(imageSize.x, imageSize.y);
                    imageSize.y *= 512.0f / DRE::Max(imageSize.x, imageSize.y);
                }


                DRE::String128 window_id{ m_DisplayedTextures[i]->GetResource()->name_ };
                window_id.Append("##wnd");

                ImGui::BeginChild(window_id.GetData(), ImVec2{}, ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border, ImGuiWindowFlags_NoMove);
                ImGui::Text(m_DisplayedTextures[i]->GetResource()->name_);
                auto handle = m_DisplayedTextures[i]->GetImGuiDescriptor().GetHandle();
                if (ImGui::ImageButton(m_DisplayedTextures[i]->GetResource()->name_.GetData(), handle, imageSize))
                {
                    deleteQueue.EmplaceBack(m_DisplayedTextures[i]);
                }
                ImGui::EndChild();

                ////////////////////////////
                // Texture View END
                ////////////////////////////
                GFX::g_GraphicsManager->GetImGuiSyncQueue().EmplaceBack(m_DisplayedTextures[i]);
            }

            for (std::uint32_t i = 0, size = deleteQueue.Size(); i < size; i++)
            {
                m_DisplayedTextures.RemoveValue(deleteQueue[i]);
            }
#endif
        }
        ImGui::EndChild();
        ////////////////////////////
        // Texture View END
        ////////////////////////////
    }

    ImGui::End();

    if (!isOpen)
    {
        Close();
    }
}

}
