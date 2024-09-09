#pragma once

#include <engine\scene\SceneNodeManipulator.hpp>

namespace SYS
{
class InputSystem;
}

namespace EDITOR
{

class ViewportInputManager
{
public:
    ViewportInputManager();

    void ProcessMouseInput(SYS::InputSystem& inputSystem);


private:
    bool TryInteractWithDebugPrimitives(SYS::InputSystem& inputSystem);

private:
    WORLD::SceneNodeManipulator m_NodeManipulator;
};

}

