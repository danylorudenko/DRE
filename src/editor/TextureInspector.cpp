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
    , m_TextureSizeMultiplier{ 0.75f }
{}

TextureInspector::TextureInspector(TextureInspector&& rhs)
    : BaseEditor{ DRE_MOVE(rhs) }
    , m_TextureBank{ nullptr }
    , m_TextureSizeMultiplier{ 0.75f }
{
    operator=(DRE_MOVE(rhs));
}

TextureInspector& TextureInspector::operator=(TextureInspector&& rhs)
{
    BaseEditor::operator=(DRE_MOVE(rhs));

    DRE_SWAP_MEMBER(m_TextureBank);
    DRE_SWAP_MEMBER(m_GraphResources);
    DRE_SWAP_MEMBER(m_TextureSizeMultiplier);

    return *this;
}

void TextureInspector::Render()
{
    ImGui::SetNextWindowSize(ImVec2(250, 250), ImGuiCond_FirstUseEver);

    bool isOpen = true;
    if (ImGui::Begin("Texture Inspector", &isOpen, ImGuiWindowFlags_None))
    {
        ////////////////////////////
        // Texture List BEGIN
        ////////////////////////////
        if (ImGui::BeginChild("nodes_tree", ImVec2(150, 0), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX))
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
                        auto& ctx = DRE::g_AppContext;
                        if (m_DisplayedTexture == &texture)
                        {
                            m_DisplayedTexture = nullptr;
                            ctx.m_TextureInspectorViewState.m_DrawTexture = false;
                            ctx.m_TextureInspectorViewState.m_TextureName.Shrink(0);
                        }
                        else
                        {
                            m_DisplayedTexture = &texture;
                            ctx.m_TextureInspectorViewState.m_DrawTexture = true;
                            ctx.m_TextureInspectorViewState.m_TextureName = m_DisplayedTexture->GetResource()->name_;

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
        };
        ImGui::EndChild();
        ////////////////////////////
        // Texture List END
        ////////////////////////////


        ImGui::SameLine();


        ////////////////////////////
        // Texture View BEGIN
        ////////////////////////////
        if (ImGui::BeginChild("properties"))
        {
#ifdef DRE_IMGUI_CUSTOM_TEXTURE

            auto& ctx = DRE::g_AppContext.m_TextureInspectorViewState;

            ImGui::TextUnformatted(ctx.m_TextureName.GetData());
            if (ImGui::Button("Disable View"))
            {
                ctx.m_DrawTexture = false;
                ctx.m_TextureName.Shrink(0);
                m_DisplayedTexture = nullptr;
            }

            ImGui::SliderFloat("Size", &ctx.m_Size, 0.0f, 1.0f);

            ImGui::SliderFloat("Lower", &ctx.m_LowerEnd, 0.0f, 1.0f);
            ImGui::SliderFloat("Upper", &ctx.m_UpperEnd, 0.0f, 1.0f);

            ImGui::Checkbox("X Channel", &ctx.m_ShowX);
            ImGui::Checkbox("Y Channel", &ctx.m_ShowY);
            ImGui::Checkbox("Z Channel", &ctx.m_ShowZ);
            ImGui::Checkbox("W Channel", &ctx.m_ShowW);


            /*
            DRE::InplaceVector<GFX::Texture*, 2> deleteQueue;
            for(std::uint32_t i = 0, size = m_DisplayedTextures.Size(); i < size; i++)
            {
                ////////////////////////////
                // Texture View BEGIN
                ////////////////////////////
                ImVec2 imageSize{ float(m_DisplayedTextures[i]->GetWidth()), float(m_DisplayedTextures[i]->GetHeight()) };
                imageSize.x *= m_TextureSizeMultiplier;
                imageSize.y *= m_TextureSizeMultiplier;



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

            */
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
