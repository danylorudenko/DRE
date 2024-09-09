#include <editor\ViewportInputManager.hpp>

#include <foundation\input\InputSystem.hpp>

namespace EDITOR
{

ViewportInputManager::ViewportInputManager()
{
}

void ViewportInputManager::ProcessMouseInput(SYS::InputSystem& inputSystem)
{
    if (!TryInteractWithDebugPrimitives(inputSystem))
    {
        // try pick another object in the scene
    }
}

bool ViewportInputManager::TryInteractWithDebugPrimitives(SYS::InputSystem& inputSystem)
{

}



}