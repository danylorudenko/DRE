#pragma once

#include <engine\scene\SceneNodeManipulator.hpp>

namespace SYS
{
class InputSystem;
}

namespace GFX
{
class RenderView;
}

namespace WORLD
{
class Scene;
}

namespace EDITOR
{

class ViewportInputManager
{
public:
    ViewportInputManager(WORLD::Scene* scene);

    void ProcessInput(SYS::InputSystem& inputSystem, GFX::RenderView& view);

    bool ShouldRenderTranslationGizmo() const;
    glm::vec3 GetFocusedObjectPosition() const;

private:
    bool TryInteractWithDebugPrimitives(SYS::InputSystem& inputSystem, GFX::RenderView& view);

private:
    WORLD::Scene* m_MainScene;
    WORLD::SceneNodeManipulator m_NodeManipulator;
};

}

