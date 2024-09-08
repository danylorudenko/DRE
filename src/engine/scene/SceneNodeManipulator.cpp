#include <engine\scene\SceneNodeManipulator.hpp>

#include <foundation\input\InputSystem.hpp>
#include <foundation\math\Geometry.hpp>

#include <gfx\view\RenderView.hpp>
#include <engine\scene\SceneNode.hpp>

namespace WORLD
{

SceneNodeManipulator::SceneNodeManipulator()
    : m_FocusedNode{ nullptr }
    , m_PlaneAxis{ Axis(0) }
    , m_DraggedAxis{ Axis(0) }
    , m_BeginTranslatePoint{ 0.0f, 0.0f, 0.0f }
    , m_CurrentTranslatePoint{ 0.0f, 0.0f, 0.0f }
{}

void SceneNodeManipulator::SetFocusedNode(SceneNode* node)
{
    m_FocusedNode = node;
}

SceneNodeManipulator::Axis SceneNodeManipulator::PickBestPlaneAxis(glm::vec3 cameraDir)
{
    float dotX = glm::abs(glm::dot(cameraDir, glm::vec3{ 1.0f, 0.0f, 0.0f }));
    float dotY = glm::abs(glm::dot(cameraDir, glm::vec3{ 0.0f, 1.0f, 0.0f }));
    float dotZ = glm::abs(glm::dot(cameraDir, glm::vec3{ 0.0f, 0.0f, 1.0f }));

    if (dotX >= dotY && dotX >= dotZ)
        return Axis::X;

    if (dotY >= dotX && dotY >= dotZ)
        return Axis::Y;

    if (dotZ >= dotY && dotZ >= dotX)
        return Axis::Z;

    return Axis::X;
}

void SceneNodeManipulator::TryInteract(SYS::InputSystem& inputSystem, GFX::RenderView& view)
{
    // I should be careful, because rn click will refocus the node

    if (inputSystem.GetLeftMouseButtonJustPressed())
        m_DraggedAxis = Axis(0.0f);

    if (m_FocusedNode == nullptr)
        return;

    if (inputSystem.GetLeftMouseButtonJustPressed())
    {
        m_DraggedAxis = Axis(0);

        glm::ivec2 mousePos{ inputSystem.GetMouseState().mousePosX_, inputSystem.GetMouseState().mousePosY_ };
        // raycast gizmo and startDragging
        DRE::Ray cameraRay = DRE::RayFromCamera(mousePos, view.GetSize(), view.GetInvViewProjectionM(), view.GetPosition());

        float const gizmoDistance = glm::length(view.GetPosition() - m_FocusedNode->GetGlobalPosition());
        float const gizmoLength = WORLD::SceneNodeManipulator::GIZMO_CYLINDER_LENGTH * gizmoDistance;
        float const gizmoRadius = WORLD::SceneNodeManipulator::GIZMO_CYLINDER_RADIUS * gizmoDistance;

        glm::vec3 gizmoPos = m_FocusedNode->GetGlobalPosition();

        DRE::Cylinder xCylinder{ gizmoPos, gizmoPos + glm::vec3{ gizmoLength, 0.0f, 0.0f }, gizmoRadius };
        DRE::Cylinder yCylinder{ gizmoPos, gizmoPos + glm::vec3{ 0.0f, gizmoLength, 0.0f }, gizmoRadius };
        DRE::Cylinder zCylinder{ gizmoPos, gizmoPos + glm::vec3{ 0.0f, 0.0f, gizmoLength }, gizmoRadius };

        float tMin = DRE_FLT_MAX;
        float t1 = 0.0, t2 = 0.0f;
        if (DRE::RayCylinderIntersection(cameraRay, xCylinder, t1, t2))
        {
            m_DraggedAxis = Axis::X;
            tMin = glm::min(t1, t2);
        }
        if (DRE::RayCylinderIntersection(cameraRay, yCylinder, t1, t2))
        {
            float t = glm::min(t1, t2);
            if (t <= tMin)
            {
                tMin = t;
                m_DraggedAxis = Axis::Y;
            }
        }
        if (DRE::RayCylinderIntersection(cameraRay, zCylinder, t1, t2))
        {
            float t = glm::min(t1, t2);
            if (t <= tMin)
            {
                tMin = t;
                m_DraggedAxis = Axis::Z;
            }
        }
    }

    if (m_DraggedAxis != Axis(0))
    {
        // continue dragging

        if (inputSystem.GetLeftMouseButtonJustReleased())
            m_DraggedAxis = Axis(0);
    }
}

}

