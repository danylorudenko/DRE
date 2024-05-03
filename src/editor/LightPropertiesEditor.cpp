#include <editor\LightPropertiesEditor.hpp>

#include <foundation\Common.hpp>

#include <editor\RootEditor.hpp>
#include <engine\scene\ISceneNodeUser.hpp>
#include <engine\ApplicationContext.hpp>
#include <engine\scene\Light.hpp>

#include <glm\gtc\type_ptr.hpp>
#include <imgui.h>

namespace EDITOR
{

LightPropertiesEditor::LightPropertiesEditor(BaseEditor* rootEditor, EditorFlags flags)
    : BaseEditor{ rootEditor, flags }
    , m_EditedLight{ nullptr }
{}

LightPropertiesEditor::LightPropertiesEditor(LightPropertiesEditor&& rhs)
    : BaseEditor{ DRE_MOVE(rhs) }
    , m_EditedLight{ nullptr }
{
    operator=(DRE_MOVE(rhs));
}

LightPropertiesEditor& LightPropertiesEditor::operator=(LightPropertiesEditor&& rhs)
{
    BaseEditor::operator=(DRE_MOVE(rhs));

    DRE_SWAP_MEMBER(m_EditedLight);

    return *this;
}

void LightPropertiesEditor::SetLight(WORLD::ISceneNodeUser* user)
{
    switch (user->GetType())
    {
    case WORLD::ISceneNodeUser::Type::Light:
        m_EditedLight = user;
        break;
    default:
        break;
    }
}

void LightPropertiesEditor::Render()
{
    bool isOpen = true;
    if (ImGui::Begin("Light Editor", &isOpen, ImGuiWindowFlags_None))
    {
        if (m_EditedLight != nullptr)
        {
            switch (m_EditedLight->GetType())
            {
            case WORLD::ISceneNodeUser::Type::Light:
            {
                RenderDirectionalLightProperties();
                break;
            }
            default:
                ImGui::Text("Unsupported light type.");
                break;
            }
        }
        else
        {
            ImGui::Text("No light was selected");
        }
    }

    ImGui::End();

    if (!isOpen)
    {
        Close();
    }
}

void LightPropertiesEditor::RenderDirectionalLightProperties()
{
    ImGui::Text("Name: %s", m_EditedLight->GetSceneNode()->GetName());
    ImGui::Text("Type: Directional Light");

    WORLD::Light* light = reinterpret_cast<WORLD::Light*>(m_EditedLight);

    bool needsUpdate = false;

    glm::vec3 spectrum = light->GetSpectrum();
    if (ImGui::DragFloat3("spectrum: ", glm::value_ptr(spectrum), 1.0f, 0.0f, 100.0f))
    {
        light->SetSpectrum(spectrum);
        needsUpdate = true;
    }

    float flux = light->GetFlux();
    if (ImGui::DragFloat("flux: ", &flux, 1.0f, 0.0f, 100.0f))
    {
        light->SetFlux(flux);
        needsUpdate = true;
    }

    glm::vec3 orientation = light->GetEulerOrientation();
    if (ImGui::DragFloat3("oreintation: ", glm::value_ptr(orientation), 1.0f, 0.0f, 360.0f))
    {
        light->SetEulerOrientation(orientation);
        needsUpdate = true;
    }

    if (needsUpdate)
    {
        light->ScheduleUpdateGPUData();
    }

}

}
