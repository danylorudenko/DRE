#include <editor\RootEditor.hpp>

#include <foundation\Common.hpp>
#include <engine\scene\Camera.hpp>
#include <engine\scene\Scene.hpp>
#include <editor\CameraEditor.hpp>
#include <editor\SceneGraphEditor.hpp>

#include <imgui.h>

namespace EDITOR
{

RootEditor::RootEditor(WORLD::Scene* mainScene)
    : BaseEditor{ nullptr, EDITOR_FLAGS_STATIC }
    , m_MainScene{ mainScene }
    , m_Editors{ &DRE::g_MainAllocator }
    , m_CloseQueue{ &DRE::g_MainAllocator }
{}

RootEditor::RootEditor(RootEditor&& rhs)
    : BaseEditor{ DRE_MOVE(rhs) }
    , m_MainScene{ nullptr }
{
    operator=(DRE_MOVE(rhs));
}

RootEditor& RootEditor::operator=(RootEditor&& rhs)
{
    BaseEditor::operator=(DRE_MOVE(rhs));

    DRE_SWAP_MEMBER(m_MainScene);
    DRE_SWAP_MEMBER(m_Editors);
    DRE_SWAP_MEMBER(m_CloseQueue);

    return *this;
}

RootEditor::~RootEditor()
{
    for(std::uint32_t i = 0, size = m_Editors.Size(); i < size; i++)
    {
        DRE::g_MainAllocator.FreeObject(m_Editors[i]);
    }
}

BaseEditor* RootEditor::GetEditorByType(BaseEditor::Type type)
{
    BaseEditor* result = nullptr;
    DRE::U32 const id = m_Editors.FindIf([type](BaseEditor* e) { return e->GetType() == type; });
    if (id != m_Editors.Size())
    {
        result = m_Editors[id];
    }

    return result;
}

void RootEditor::Render()
{
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_Modal |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoDecoration;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("General Editors"))
        {
            if (ImGui::MenuItem("Camera Editor"))
            {
                if (GetEditorByType(BaseEditor::Type::Camera) == nullptr)
                {
                    CameraEditor* cameraEditor = DRE::g_MainAllocator.Alloc<CameraEditor>(this, EDITOR_FLAGS_STATIC, &m_MainScene->GetMainCamera());
                    m_Editors.EmplaceBack(cameraEditor);
                }
            }

            if (ImGui::MenuItem("Scene Graph Editor"))
            {
                if (GetEditorByType(BaseEditor::Type::SceneGraph) == nullptr)
                {
                    SceneGraphEditor* sceneEditor = DRE::g_MainAllocator.Alloc<SceneGraphEditor>(this, EDITOR_FLAGS_NONE, m_MainScene);
                    m_Editors.EmplaceBack(sceneEditor);
                }
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    for (std::uint32_t i = 0, size = m_Editors.Size(); i < size; i++)
    {
        m_Editors[i]->Render();
    }


    if (!m_CloseQueue.Empty())
    {
        for (DRE::U32 i = 0, size = m_CloseQueue.Size(); i < size; i++)
        {
            DRE::g_MainAllocator.FreeObject(m_CloseQueue[i]);

            DRE::U32 const result = m_Editors.Find(m_CloseQueue[i]);
            DRE_ASSERT(result != m_Editors.Size(), "Can't find the editor to delete.");
            if (result != m_Editors.Size())
            {
                m_Editors.RemoveIndex(result);
            }
        }

        m_CloseQueue.Clear();
    }
    
}

void RootEditor::Close()
{
    // can't be closed
}

void RootEditor::CloseEditor(BaseEditor* editor)
{
    m_CloseQueue.EmplaceBack(editor);
}



}