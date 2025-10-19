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
        if (GFX::g_GraphicsManager != nullptr)
        {
            GFX::GraphicsSettings& settings = GFX::g_GraphicsManager->GetGraphicsSettings();

            ImGui::Checkbox("Use ACES", &settings.m_UseACESEncoding);
            ImGui::SliderFloat("Exposure target EV", &settings.m_ExposureEV, -3.0f, 5.0f);

            if (ImGui::Button("Enable TAA"))
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

            ImGui::Checkbox("Pause time", &DRE::g_AppContext.m_PauseTime);

            ImGui::SliderFloat("Generic Scalar", &settings.m_GenericScalar, -2.0f, 2.0f);

            ImGui::Separator();
            ImGui::TextUnformatted("Water settings");

            ImGui::Checkbox("Water wireframe", &settings.m_WaterWireframe);
            ImGui::Checkbox("FFT Water", &settings.m_UseFFTWater);
            ImGui::SliderFloat("Water speed", &settings.m_WaterSpeed, 0.0f, 3.0f);
            ImGui::SliderFloat("Water size meters", &settings.m_WaterSizeMeters, 1.0f, 100.0f);
            ImGui::SliderFloat("Water amplitude", &settings.m_WaterAmplitude, 0.0f, 1000.0f);
            ImGui::SliderFloat("Water wind dir X", &settings.m_WindDirectionX, -1.0f, 1.0f);
            ImGui::SliderFloat("Water wind speed", &settings.m_WindSpeed, 0.0f, 100.0f);
            ImGui::SliderFloat("Water wind dir factor", &settings.m_WindDirFactor, 0.0f, 100.0f);
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
