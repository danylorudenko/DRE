#include <editor\SceneGraphEditor.hpp>

#include <foundation\Common.hpp>
#include <foundation\string\InplaceString.hpp>

#include <editor\RootEditor.hpp>
#include <engine\scene\Scene.hpp>
#include <engine\scene\ISceneNodeUser.hpp>
#include <engine\ApplicationContext.hpp>

#include <glm\gtc\type_ptr.hpp>

#include <imgui.h>

namespace EDITOR
{

SceneGraphEditor::SceneGraphEditor(BaseEditor* rootEditor, EditorFlags flags, WORLD::Scene* scene)
    : BaseEditor{ rootEditor, flags }
    , m_Scene{ scene }
{}

SceneGraphEditor::SceneGraphEditor(SceneGraphEditor&& rhs)
    : BaseEditor{ DRE_MOVE(rhs) }
    , m_Scene{ nullptr }
{
    operator=(DRE_MOVE(rhs));
}

SceneGraphEditor& SceneGraphEditor::operator=(SceneGraphEditor&& rhs)
{
    BaseEditor::operator=(DRE_MOVE(rhs));

    DRE_SWAP_MEMBER(m_Scene);

    return *this;
}

void SceneGraphEditor::Render()
{
    ImGui::SetNextWindowSize(ImVec2(500, 440), ImGuiCond_FirstUseEver);

    bool isOpen = true;
    if (ImGui::Begin("Scene Graph Editor", &isOpen, ImGuiWindowFlags_None))
    {
        if (ImGui::BeginChild("nodes_tree", ImVec2(150, 0), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX))
        {
            RenderingContext context;

            WORLD::SceneNode* root = m_Scene->GetRootNode();
            DRE::U32 const rootChildCount = root->GetChildrenCount();
            for (DRE::U32 i = 0; i < rootChildCount; ++i)
            {
                RenderSceneNodeRecursive(root->GetChild(i), context);
            }
        };
        ImGui::EndChild();

        ImGui::SameLine();

        if (ImGui::BeginChild("props_window"))
        {
            if (DRE::g_AppContext.m_FocusedObject != nullptr)
            {
                bool transformUpdated = RenderNodeProperties();
                if (DRE::g_AppContext.m_FocusedObject->GetType() == WORLD::Entity::Type::Light)
                {
                    RenderLightProperties(transformUpdated);
                }
            }
            else
            {
                ImGui::TextDisabled("No Node selected");
            }
        }
        ImGui::EndChild();
    }

    ImGui::End();

    if (!isOpen)
    {
        Close();
    }
}

bool SceneGraphEditor::RenderNodeProperties()
{
    bool wasUpdated = false;

    char const* name = DRE::g_AppContext.m_FocusedObject->GetSceneNode()->GetName();
    glm::vec3 pos = DRE::g_AppContext.m_FocusedObject->GetPosition();
    glm::vec3 rot = DRE::g_AppContext.m_FocusedObject->GetEulerOrientation();
    float scale = DRE::g_AppContext.m_FocusedObject->GetScale();

    ImGui::Text("Selected Object: %s", name);
    ImGui::Separator();

    ImGui::SeparatorText("Transform");

    ImGui::NewLine();
    ImGui::Text("Position:");
    if (ImGui::DragFloat3("##pos", glm::value_ptr(pos), 1.0f))
    {
        DRE::g_AppContext.m_FocusedObject->SetPosition(rot);
        wasUpdated = true;
    }

    ImGui::NewLine();
    ImGui::Text("Orientation:");
    if (ImGui::DragFloat3("##rot", glm::value_ptr(rot), 1.0f))
    {
        DRE::g_AppContext.m_FocusedObject->SetEulerOrientation(rot);
        wasUpdated = true;
    }

    ImGui::NewLine();
    ImGui::Text("Scale:");
    if (ImGui::DragFloat("##scale", &scale, 0.1f))
    {
        DRE::g_AppContext.m_FocusedObject->SetScale(scale);
        wasUpdated = true;
    }

    return wasUpdated;
}

bool SceneGraphEditor::RenderLightProperties(bool wasTransformUpdated)
{
    ImGui::NewLine();
    ImGui::SeparatorText("Light");

    WORLD::Light* light = reinterpret_cast<WORLD::Light*>(DRE::g_AppContext.m_FocusedObject);
    bool needsUpdate = wasTransformUpdated;

    int currentType = light->GetLightType();
    glm::vec3 spectrum = light->GetSpectrum();
    float flux = light->GetFlux();


    // go and see "shaders\lights.h" for proper order
    const char* items[] = { "Directional", "Point" };
    static_assert(IM_ARRAYSIZE(items) == DRE_LIGHT_TYPE_MAX, "Mismatch in the editor Light type array and engine DEFINES");
    ImGui::Text("Type:");
    if (ImGui::Combo("##type", &currentType, items, IM_ARRAYSIZE(items)))
    {
        light->SetLightType(currentType);
        needsUpdate = true;
    }

    ImGui::NewLine();
    ImGui::Text("Spectrum:");
    if (ImGui::DragFloat3("##spectrum", glm::value_ptr(spectrum), 1.0f, 0.0f, 100.0f))
    {
        light->SetSpectrum(spectrum);
        needsUpdate = true;
    }

    ImGui::NewLine();
    ImGui::Text("Flux:");
    if (ImGui::DragFloat("##flux", &flux, 1.0f, 0.0f, 100.0f))
    {
        light->SetFlux(flux);
        needsUpdate = true;
    }

    if (needsUpdate)
    {
        light->ScheduleUpdateGPUData();
    }

    return needsUpdate;
}

DRE::String64 SceneGraphEditor::GetUniqueLabel(WORLD::SceneNode* node, SceneGraphEditor::RenderingContext& context)
{
    char const* typeStr = "Node";
    if (node->GetNodeUser() != nullptr) // not a structural node
    {
        switch (node->GetNodeUser()->GetType())
        {
        case WORLD::ISceneNodeUser::Type::Entity:
            typeStr = "Entity";
            break;
        case WORLD::ISceneNodeUser::Type::Camera:
            typeStr = "Camera";
            break;
        case WORLD::ISceneNodeUser::Type::Light:
            typeStr = "Directional Light";
        }
    }

    char buffer[64];
    char const* label = std::strlen(node->GetName()) > 0 ? node->GetName() : typeStr;
    std::sprintf(buffer, "%s##%u", label, context.m_CurrentID++);

    return DRE::String64{ buffer };
}

void SceneGraphEditor::RenderSceneNodeRecursive(WORLD::SceneNode* node, SceneGraphEditor::RenderingContext& context)
{
    DRE::U32 const childCount = node->GetChildrenCount();
    DRE::String64 label = GetUniqueLabel(node, context);

    ImGuiTreeNodeFlags const flags = childCount > 0 ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_Leaf;
    if (ImGui::TreeNodeEx(label.GetData(), flags))
    {
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            DRE::g_AppContext.m_FocusedObject = node->GetNodeUser();
        }

        for (DRE::U32 i = 0; i < childCount; ++i)
        {
            RenderSceneNodeRecursive(node->GetChild(i), context);
        }

        ImGui::TreePop();
    }
}

}
