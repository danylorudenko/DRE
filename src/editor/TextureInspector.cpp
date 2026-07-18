#include <editor\TextureInspector.hpp>

#include <foundation\Common.hpp>
#include <foundation\string\InplaceString.hpp>

#include <editor\RootEditor.hpp>
#include <engine\ApplicationContext.hpp>

#include <gfx\GraphicsManager.hpp>

#include <imgui.h>

namespace EDITOR
{

TextureInspector::TextureInspector(BaseEditor* rootEditor, EditorFlags flags, GFX::TextureBank* bank, GFX::GraphResourcesManager* graphResources)
    : BaseEditor{ rootEditor, flags }
    , m_TextureBank{ bank }
    , m_GraphResources{ graphResources }
    , m_DisplayedTexture{ nullptr }
    , m_TextureSizeMultiplier{ 0.75f }
    , m_TextureLowerEnd{ 0.0f }
    , m_TextureUpperEnd{ 1.0f }
    , m_ShowX{ true }
    , m_ShowY{ true }
    , m_ShowZ{ true }
    , m_ShowW{ true }
{}

TextureInspector::TextureInspector(TextureInspector&& rhs)
    : BaseEditor{ DRE_MOVE(rhs) }
    , m_TextureBank{ nullptr }
    , m_GraphResources{ nullptr }
    , m_DisplayedTexture{ nullptr }
    , m_TextureSizeMultiplier{ 0.75f }
    , m_TextureLowerEnd{ 0.0f }
    , m_TextureUpperEnd{ 1.0f }
    , m_ShowX{ true }
    , m_ShowY{ true }
    , m_ShowZ{ true }
    , m_ShowW{ true }
{
    operator=(DRE_MOVE(rhs));
}

TextureInspector& TextureInspector::operator=(TextureInspector&& rhs)
{
    BaseEditor::operator=(DRE_MOVE(rhs));

    DRE_SWAP_MEMBER(m_TextureBank);
    DRE_SWAP_MEMBER(m_GraphResources);
    DRE_SWAP_MEMBER(m_TextureSizeMultiplier);
    DRE_SWAP_MEMBER(m_TextureLowerEnd);
    DRE_SWAP_MEMBER(m_TextureUpperEnd);
    DRE_SWAP_MEMBER(m_ShowX);
    DRE_SWAP_MEMBER(m_ShowY);
    DRE_SWAP_MEMBER(m_ShowZ);
    DRE_SWAP_MEMBER(m_ShowW);
    DRE_SWAP_MEMBER(m_DisplayedTexture);

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

            auto TextureInListGraph = [this, &searchBuff, searchLength](DRE::String64& textureId)
            {
                if (searchLength == 0 || std::strstr(textureId.GetData(), searchBuff) != nullptr)
                {
                    GFX::GraphResourcesManager::AccumulatedInfo const* info = m_GraphResources->GetAccumulatedTextureInfo(textureId);
                    if (info == nullptr)
                    {
                        return;
                    }

                    auto& ctx = DRE::g_AppContext.m_TextureInspectorViewState;
                    GFX::Texture* texture = nullptr;
                    if (info->flags & GFX::GraphResourceFlags::TEMPORAL)
                    {
                        if (ctx.m_Flags & DRE::TextureViewState::FLAG_SHOW_HISTORY)
                        {
                            texture = m_GraphResources->GetTemporalTexture(textureId, GFX::g_GraphicsManager->GetPrevFrameID());
                        }
                        else
                        {
                            texture = m_GraphResources->GetTemporalTexture(textureId, GFX::g_GraphicsManager->GetCurrentFrameID());
                        }
                    }
                    else
                    {
                        texture = m_GraphResources->GetTexture(textureId);
                    }

                    if (ImGui::Button(textureId.GetData()))
                    {
                        if (m_DisplayedTexture == texture)
                        {
                            m_DisplayedTexture = nullptr;
                            DRE::ClearFlag32(ctx.m_Flags, DRE::TextureViewState::FLAG_DRAW);
                            ctx.m_TextureName.Shrink(0);
                        }
                        else
                        {
                            m_DisplayedTexture = texture;
                            DRE::SetFlag32(ctx.m_Flags, DRE::TextureViewState::FLAG_DRAW);
                            ctx.m_TextureName = textureId;

                        }
                    }
                }
            };

            auto TextureInListDisk = [this, &searchBuff, searchLength](GFX::Texture& texture)
            {
                char const* name = texture.GetShaderView()->parentResource_->name_.GetData();
                if (searchLength == 0 || std::strstr(name, searchBuff) != nullptr)
                {
                    if (ImGui::Button(name))
                    {
                        auto& ctx = DRE::g_AppContext.m_TextureInspectorViewState;
                        if (m_DisplayedTexture == &texture)
                        {
                            m_DisplayedTexture = nullptr;
                            DRE::ClearFlag32(ctx.m_Flags, DRE::TextureViewState::FLAG_DRAW);
                            ctx.m_TextureName.Shrink(0);
                        }
                        else
                        {
                            m_DisplayedTexture = &texture;
                            DRE::SetFlag32(ctx.m_Flags, DRE::TextureViewState::FLAG_DRAW);
                            ctx.m_TextureName = m_DisplayedTexture->GetResource()->name_;

                        }
                    }
                }
            };

            ImGui::SeparatorText("Graph Textures");
            {
                m_GraphResources->ForEachTexture([&TextureInListGraph](auto& texture)
                {
                    TextureInListGraph(texture.value->id);
                });
            }

            ImGui::SeparatorText("Disc Textures");
            {
                m_TextureBank->ForEachTexture([&TextureInListDisk](auto& texture)
                {
                    TextureInListDisk(*texture.value);
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
                DRE::ClearFlag32(ctx.m_Flags, DRE::TextureViewState::FLAG_DRAW);
                ctx.m_TextureName.Shrink(0);
                m_DisplayedTexture = nullptr;
            }

            bool showHistory = DRE::IsFlagSet32(ctx.m_Flags, DRE::TextureViewState::FLAG_SHOW_HISTORY);
            ImGui::Checkbox("Show History", &showHistory);
            if (showHistory) { DRE::SetFlag32(ctx.m_Flags, DRE::TextureViewState::FLAG_SHOW_HISTORY); } else { DRE::ClearFlag32(ctx.m_Flags, DRE::TextureViewState::FLAG_SHOW_HISTORY); }

            ImGui::SliderFloat("Size X", &ctx.m_SizeX, 0.0f, 50.0f);
            ImGui::SliderFloat("Size Y", &ctx.m_SizeY, 0.0f, 50.0f);
            ImGui::SliderFloat("Offset X", &ctx.m_OffsetX, -50.0f, 50.0f);
            ImGui::SliderFloat("Offset Y", &ctx.m_OffsetY, -50.0f, 50.0f);

            ImGui::SliderFloat("Lower", &ctx.m_LowerEnd, -5.0f, 5.0f);
            ImGui::SliderFloat("Upper", &ctx.m_UpperEnd, -5.0f, 5.0f);

            bool showXYZW[] = { DRE::IsFlagSet32(ctx.m_Flags, DRE::TextureViewState::FLAG_SHOW_X),
                                DRE::IsFlagSet32(ctx.m_Flags, DRE::TextureViewState::FLAG_SHOW_Y),
                                DRE::IsFlagSet32(ctx.m_Flags, DRE::TextureViewState::FLAG_SHOW_Z),
                                DRE::IsFlagSet32(ctx.m_Flags, DRE::TextureViewState::FLAG_SHOW_W) };

            ImGui::Checkbox("X Channel", &showXYZW[0]);
            ImGui::Checkbox("Y Channel", &showXYZW[1]);
            ImGui::Checkbox("Z Channel", &showXYZW[2]);
            ImGui::Checkbox("W Channel", &showXYZW[3]);

            // Update flags based on checkbox states
            if (showXYZW[0]) { DRE::SetFlag32(ctx.m_Flags, DRE::TextureViewState::FLAG_SHOW_X); } else { DRE::ClearFlag32(ctx.m_Flags, DRE::TextureViewState::FLAG_SHOW_X); }
            if (showXYZW[1]) { DRE::SetFlag32(ctx.m_Flags, DRE::TextureViewState::FLAG_SHOW_Y); } else { DRE::ClearFlag32(ctx.m_Flags, DRE::TextureViewState::FLAG_SHOW_Y); }
            if (showXYZW[2]) { DRE::SetFlag32(ctx.m_Flags, DRE::TextureViewState::FLAG_SHOW_Z); } else { DRE::ClearFlag32(ctx.m_Flags, DRE::TextureViewState::FLAG_SHOW_Z); }
            if (showXYZW[3]) { DRE::SetFlag32(ctx.m_Flags, DRE::TextureViewState::FLAG_SHOW_W); } else { DRE::ClearFlag32(ctx.m_Flags, DRE::TextureViewState::FLAG_SHOW_W); }
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
