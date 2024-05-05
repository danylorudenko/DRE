#include <editor\StatsEditor.hpp>

#include <editor\RootEditor.hpp>
#include <foundation\Common.hpp>
#include <engine\ApplicationContext.hpp>

#include <imgui.h>

namespace EDITOR
{

StatsEditor::StatsEditor(BaseEditor* rootEditor, EditorFlags flags)
    : BaseEditor{ rootEditor, flags }
{}

StatsEditor::StatsEditor(StatsEditor&& rhs)
    : BaseEditor{ DRE_MOVE(rhs) }
{
    operator=(DRE_MOVE(rhs));
}

StatsEditor& StatsEditor::operator=(StatsEditor&& rhs)
{
    BaseEditor::operator=(DRE_MOVE(rhs));

    return *this;
}

void StatsEditor::Render()
{
    bool isOpen = true;
    if (ImGui::Begin("Stats", &isOpen))
    {
        ImGui::Text("DT: %f ms", static_cast<double>(DRE::g_AppContext.m_DeltaTimeUS) / 1000);
        ImGui::Text("FPS: %f", 1.0 / (static_cast<double>(DRE::g_AppContext.m_DeltaTimeUS) / 1000000));
        ImGui::Text("Global T(s): %f", static_cast<double>(DRE::g_AppContext.m_TimeSinceStartUS) / 1000000);
    }
    ImGui::End();

    if (!isOpen)
    {
        Close();
    }
}

}