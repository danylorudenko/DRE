#include <editor\RenderingSettingsEditor.hpp>

#include <editor\RootEditor.hpp>
#include <foundation\Common.hpp>
#include <gfx\GraphicsManager.hpp>
#include <engine\ApplicationContext.hpp>

#include <imgui.h>

namespace EDITOR
{

RenderingSettingsEditor::RenderingSettingsEditor(BaseEditor* rootEditor, EditorFlags flags)
    : BaseEditor{ rootEditor, flags }
{}

RenderingSettingsEditor::RenderingSettingsEditor(RenderingSettingsEditor&& rhs)
    : BaseEditor{ DRE_MOVE(rhs) }
{
    operator=(DRE_MOVE(rhs));
}

RenderingSettingsEditor& RenderingSettingsEditor::operator=(RenderingSettingsEditor&& rhs)
{
    BaseEditor::operator=(DRE_MOVE(rhs));

    return *this;
}

void RenderingSettingsEditor::Render()
{
    bool isOpen = true;
    if (ImGui::Begin("Rendering Settings", &isOpen))
    {
        GFX::GraphicsManager* graphicsManager = GFX::g_GraphicsManager;
        if (graphicsManager != nullptr)
        {
            GFX::GraphicsSettings& settings = graphicsManager->GetGraphicsSettings();

            ImGui::TextUnformatted("Tone mapping");
            ImGui::Separator();
            ImGui::Checkbox("Use ACES", &settings.m_UseACESEncoding);
            ImGui::SliderFloat("Exposure target EV", &settings.m_ExposureEV, -3.0f, 5.0f);

            ImGui::Separator();
            ImGui::TextUnformatted("Temporal AA");
            if (ImGui::Button("Enable TAA defaults"))
            {
                settings.m_AlphaTAA = 0.9f;
                settings.m_JitterScale = 0.35f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Disable TAA"))
            {
                settings.m_AlphaTAA = 0.0f;
                settings.m_JitterScale = 0.0f;
            }
            ImGui::SliderFloat("TAA Alpha", &settings.m_AlphaTAA, 0.0f, 1.0f);
            ImGui::SliderFloat("TAA Jitter Scale", &settings.m_JitterScale, 0.0f, 2.0f);
            ImGui::SliderFloat("TAA Variance Gamma", &settings.m_VarianceGammaTAA, 0.0f, 5.0f);

            ImGui::Separator();
            ImGui::TextUnformatted("Timing");
            ImGui::Checkbox("Pause time", &DRE::g_AppContext.m_PauseTime);

            ImGui::Separator();
            ImGui::TextUnformatted("Misc");
            ImGui::SliderFloat("Generic Scalar", &settings.m_GenericScalar, -2.0f, 2.0f);

            ImGui::Separator();
            ImGui::TextUnformatted("Ambient Occlusion");

            // update together with ambient_occlusion_type.h
            char const* AOVersionNames[] = { "Dot Based", "Hotizon Based", "GTAO"};
            static_assert(IM_ARRAYSIZE(AOVersionNames) == AOVersion::AO_COUNT, "AOVersion doesn't correspond to the list of names");

            AOVersion* aoVersion = &settings.m_AOVersion;
            if (ImGui::BeginCombo("AO Version", AOVersionNames[*aoVersion]))
            {
                for (DRE::U32 i = 0; i < DRE::U32(AOVersion::AO_COUNT); i++)
                {
                    bool isSelected = *aoVersion == i;
                    if (ImGui::Selectable(AOVersionNames[i], &isSelected))
                    {
                        *aoVersion = (AOVersion)i;
                    }

                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }


            ImGui::SliderFloat("AO Strength", &settings.m_AOStrength, 0.0f, 2.0f);
            ImGui::SliderFloat("AO Kernel Scale", &settings.m_AOKernelScale, 0.0f, 10.0f);
            ImGui::SliderFloat("AO Max Occlusion Dist", &settings.m_AOMaxOcclusionDistance, 0.0f, 10.0f);

            ImGui::Separator();
            ImGui::TextUnformatted("DDGI");
            ImGui::SliderFloat("DDGI Probe World Distance", &settings.m_DDGIProbeWorldDistance, 0.1f, 10.0f);
            ImGui::SliderFloat3("DDGI Probe World Offset", &settings.m_DDGIProbeWorldOffset.x, -50.0f, 50.0f);
        }
        else
        {
            ImGui::TextUnformatted("Graphics manager unavailable.");
        }
    }
    ImGui::End();

    if (!isOpen)
    {
        Close();
    }
}

}
