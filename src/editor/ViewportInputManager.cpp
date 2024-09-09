#include <editor\ViewportInputManager.hpp>

#include <foundation\input\InputSystem.hpp>
#include <engine\scene\Scene.hpp>
#include <engine\ApplicationContext.hpp>

namespace EDITOR
{

ViewportInputManager::ViewportInputManager(WORLD::Scene* scene)
    : m_MainScene{ scene }
{
}

void ViewportInputManager::ProcessInput(SYS::InputSystem& inputSystem, GFX::RenderView& view)
{
    if (!TryInteractWithDebugPrimitives(inputSystem, view))
    {
        if (inputSystem.GetLeftMouseButtonJustPressed() && DRE::g_AppContext.m_MouseHoveredObjectID != 0)
        {
            WORLD::SceneNode* result = m_MainScene->GetRootNode()->FindChildByID(DRE::g_AppContext.m_MouseHoveredObjectID);
            DRE_ASSERT(result != nullptr, "Can't find picked object.");
            if (result != nullptr)
            {
                DRE::g_AppContext.m_FocusedObject = result->GetNodeUser();
                m_NodeManipulator.SetFocusedNode(DRE::g_AppContext.m_FocusedObject->GetSceneNode());
            }
        }
    }
}

bool ViewportInputManager::TryInteractWithDebugPrimitives(SYS::InputSystem& inputSystem, GFX::RenderView& view)
{
    return m_NodeManipulator.TryInteract(inputSystem, view);
}

bool ViewportInputManager::ShouldRenderTranslationGizmo() const
{
    return DRE::g_AppContext.m_FocusedObject != nullptr;
}

glm::vec3 ViewportInputManager::GetFocusedObjectPosition() const
{
    return DRE::g_AppContext.m_FocusedObject->GetGlobalPosition();
}



}