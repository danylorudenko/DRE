#include <editor\SceneGraphEditor.hpp>

#include <foundation\Common.hpp>
#include <foundation\string\InplaceString.hpp>

#include <editor\RootEditor.hpp>
#include <editor\LightPropertiesEditor.hpp>
#include <editor\TransformEditor.hpp>
#include <engine\scene\Scene.hpp>
#include <engine\scene\ISceneNodeUser.hpp>
#include <engine\ApplicationContext.hpp>


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
    bool isOpen = true;
    if (ImGui::Begin("Scene Graph Editor", &isOpen, ImGuiWindowFlags_None))
    {
        RenderingContext context;

        WORLD::SceneNode* root = m_Scene->GetRootNode();
        DRE::U32 const rootChildCount = root->GetChildrenCount();
        for (DRE::U32 i = 0; i < rootChildCount; ++i)
        {
            RenderSceneNodeRecursive(root->GetChild(i), context);
        }
    }

    ImGui::End();

    if (!isOpen)
    {
        Close();
    }
}

DRE::String64 SceneGraphEditor::GetUniqueLabel(WORLD::SceneNode* node, SceneGraphEditor::RenderingContext& context)
{
    char const* typeStr = "";
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
            AttemptOpenProperties(node);
        }

        for (DRE::U32 i = 0; i < childCount; ++i)
        {
            RenderSceneNodeRecursive(node->GetChild(i), context);
        }

        ImGui::TreePop();
    }
}

void SceneGraphEditor::AttemptOpenProperties(WORLD::SceneNode* node)
{
    RootEditor* rootEditor = reinterpret_cast<RootEditor*>(m_RootEditor);

    auto* transformEditor = reinterpret_cast<TransformEditor*>(rootEditor->GetEditorByType(BaseEditor::Type::Transform));
    if (transformEditor != nullptr)
    {
        transformEditor->SetNode(node->GetNodeUser());
    }

    switch (node->GetNodeUser()->GetType())
    {
    case WORLD::ISceneNodeUser::Type::Light:
    {
        auto* lightEditor = reinterpret_cast<LightPropertiesEditor*>(rootEditor->GetEditorByType(BaseEditor::Type::LightProperties));
        if (lightEditor != nullptr)
        {
            lightEditor->SetLight(node->GetNodeUser());
        }
        break;
    }
    }
}

}
