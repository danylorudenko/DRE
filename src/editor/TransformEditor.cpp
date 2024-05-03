#include <editor\TransformEditor.hpp>

#include <foundation\Common.hpp>

#include <editor\RootEditor.hpp>
#include <engine\scene\ISceneNodeUser.hpp>
#include <engine\ApplicationContext.hpp>
#include <engine\scene\Light.hpp>

#include <glm\gtc\type_ptr.hpp>
#include <imgui.h>

namespace EDITOR
{

TransformEditor::TransformEditor(BaseEditor* rootEditor, EditorFlags flags)
    : BaseEditor{ rootEditor, flags }
    , m_EditedNode{ nullptr }
{}

TransformEditor::TransformEditor(TransformEditor&& rhs)
    : BaseEditor{ DRE_MOVE(rhs) }
    , m_EditedNode{ nullptr }
{
    operator=(DRE_MOVE(rhs));
}

TransformEditor& TransformEditor::operator=(TransformEditor&& rhs)
{
    BaseEditor::operator=(DRE_MOVE(rhs));

    DRE_SWAP_MEMBER(m_EditedNode);

    return *this;
}

void TransformEditor::SetNode(WORLD::ISceneNodeUser* user)
{
    m_EditedNode = user;
}

void TransformEditor::Render()
{
    bool isOpen = true;
    if (ImGui::Begin("Transform Editor", &isOpen, ImGuiWindowFlags_None))
    {
        if (m_EditedNode != nullptr)
        {
            RenderTransformProperties();
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

void TransformEditor::RenderTransformProperties()
{
    ImGui::Text("To be implemented.");
}

}
