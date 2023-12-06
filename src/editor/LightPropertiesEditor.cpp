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
    case WORLD::ISceneNodeUser::Type::DirectionalLight:
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
            case WORLD::ISceneNodeUser::Type::DirectionalLight:
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

    WORLD::DirectionalLight* light = reinterpret_cast<WORLD::DirectionalLight*>(m_EditedLight);

    glm::vec3 radiance = light->GetRadiance();
    ImGui::DragFloat3("Radiance: ", glm::value_ptr(radiance), 1.0f, 0.0f, 100.0f, "%.3f");
    light->SetRadiance(radiance);

}

}
