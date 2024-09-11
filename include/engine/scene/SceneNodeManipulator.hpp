#pragma once

#include <glm\vec3.hpp>
#include <glm\vec2.hpp>

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

class SceneNode;

class SceneNodeManipulator
{
public:
    static float constexpr GIZMO_CYLINDER_RADIUS = 0.01f;
    static float constexpr GIZMO_CYLINDER_LENGTH = 0.2f;


    SceneNodeManipulator();

    void SetFocusedNode(SceneNode* node);
    bool TryInteract(SYS::InputSystem& inputSystem, GFX::RenderView& view);

private:

private:
    enum Axis
    {
        X = 1,
        Y = 1 << 1,
        Z = 1 << 2
    };
    static Axis PickBestPlaneAxis(glm::vec3 cameraDir);
    static glm::vec3 AxisToVector(SceneNodeManipulator::Axis axis);

    SceneNode*  m_FocusedNode;

    Axis        m_PlaneAxis;
    Axis        m_DraggedAxis;
    glm::vec3   m_BeginTranslatePoint;
    glm::vec3   m_BeginNodePosition;


};

}

