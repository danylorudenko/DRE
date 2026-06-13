#include <editor\DebugViewEditor.hpp>

#include <foundation\Common.hpp>
#include <engine\ApplicationContext.hpp>

#include <imgui.h>

namespace EDITOR
{

DebugViewEditor::DebugViewEditor(BaseEditor* rootEditor, EditorFlags flags)
    : BaseEditor{ rootEditor, flags }
{}

DebugViewEditor::DebugViewEditor(DebugViewEditor&& rhs)
    : BaseEditor{ DRE_MOVE(rhs) }
{
    operator=(DRE_MOVE(rhs));
}

DebugViewEditor& DebugViewEditor::operator=(DebugViewEditor&& rhs)
{
    BaseEditor::operator=(DRE_MOVE(rhs));

    return *this;
}

void DebugViewEditor::Render()
{
    bool isOpen = true;
    if (ImGui::Begin("Debug View", &isOpen))
    {
        DRE::DDGIDebugState& state = DRE::g_AppContext.m_DDGIDebugState;

        ImGui::TextUnformatted("DDGI Probes");
        ImGui::Separator();
        ImGui::Checkbox("Draw Probes", &state.m_DrawProbes);
        ImGui::SliderFloat("Sphere Scale", &state.m_SphereScale, 0.1f, 5.0f);

        ImGui::Separator();
        ImGui::TextUnformatted("Visualization Mode");
        char const* visModeNames[] = { "Color", "Normal", "UV" };
        DRE::S32 currentMode = static_cast<DRE::S32>(state.m_VisMode);
        if (ImGui::BeginCombo("Vis Mode", visModeNames[currentMode]))
        {
            for (DRE::S32 i = 0; i < IM_ARRAYSIZE(visModeNames); i++)
            {
                bool isSelected = (currentMode == i);
                if (ImGui::Selectable(visModeNames[i], &isSelected))
                    state.m_VisMode = static_cast<DRE::DDGIDebugState::VisMode>(i);
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }
    ImGui::End();

    if (!isOpen)
        Close();
}

}
