#include <editor\RootEditor.hpp>

#include <foundation\Common.hpp>
#include <engine\scene\Camera.hpp>
#include <engine\scene\Scene.hpp>
#include <editor\CameraEditor.hpp>
#include <editor\RenderingSettingsEditor.hpp>
#include <editor\DebugViewEditor.hpp>
#include <editor\SceneGraphEditor.hpp>
#include <editor\StatsEditor.hpp>
#include <editor\TextureInspector.hpp>

#include <gfx\GraphicsManager.hpp>

#include <imgui.h>

namespace EDITOR
{

RootEditor::RootEditor(WORLD::Scene* mainScene, ViewportInputManager* inputManager)
    : BaseEditor{ nullptr, EDITOR_FLAGS_STATIC }
    , m_MainScene{ mainScene }
    , m_ViewportInputManager{ inputManager }
    , m_Editors{ &DRE::g_PersistentDataAllocator }
    , m_CloseQueue{ &DRE::g_FrameScratchAllocator }
{}

RootEditor::RootEditor(RootEditor&& rhs)
    : BaseEditor{ DRE_MOVE(rhs) }
    , m_MainScene{ nullptr }
    , m_ViewportInputManager{ nullptr }
{
    operator=(DRE_MOVE(rhs));
}

RootEditor& RootEditor::operator=(RootEditor&& rhs)
{
    BaseEditor::operator=(DRE_MOVE(rhs));

    DRE_SWAP_MEMBER(m_MainScene);
    DRE_SWAP_MEMBER(m_ViewportInputManager);
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
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::MenuItem("Statistics"))
        {
            if (GetEditorByType(BaseEditor::Type::Stats) == nullptr)
            {
                StatsEditor* statsEditor = DRE::g_MainAllocator.Alloc<StatsEditor>(this, EDITOR_FLAGS_NONE);
                m_Editors.EmplaceBack(statsEditor);
            }
        }

        if (ImGui::MenuItem("Rendering Settings"))
        {
            if (GetEditorByType(BaseEditor::Type::RenderingSettings) == nullptr)
            {
                RenderingSettingsEditor* renderingEditor = DRE::g_MainAllocator.Alloc<RenderingSettingsEditor>(this, EDITOR_FLAGS_NONE);
                m_Editors.EmplaceBack(renderingEditor);
            }
        }

        if (ImGui::MenuItem("Debug View"))
        {
            if (GetEditorByType(BaseEditor::Type::DebugView) == nullptr)
            {
                DebugViewEditor* debugViewEditor = DRE::g_MainAllocator.Alloc<DebugViewEditor>(this, EDITOR_FLAGS_NONE);
                m_Editors.EmplaceBack(debugViewEditor);
            }
        }

        if (ImGui::MenuItem("Texture Inspector"))
        {
            if (GetEditorByType(BaseEditor::Type::TextureInspector) == nullptr)
            {
                TextureInspector* statsEditor = DRE::g_MainAllocator.Alloc<TextureInspector>(this, EDITOR_FLAGS_NONE, &GFX::g_GraphicsManager->GetTextureBank(), &GFX::g_GraphicsManager->GetMainRenderGraph().GetResourcesManager());
                m_Editors.EmplaceBack(statsEditor);
            }
        }

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
                    SceneGraphEditor* sceneEditor = DRE::g_MainAllocator.Alloc<SceneGraphEditor>(this, m_ViewportInputManager, EDITOR_FLAGS_NONE, m_MainScene);
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