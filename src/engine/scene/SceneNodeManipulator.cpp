#include <engine\scene\SceneNodeManipulator.hpp>

#include <foundation\input\InputSystem.hpp>
#include <foundation\math\Geometry.hpp>

#include <gfx\view\RenderView.hpp>
#include <engine\scene\SceneNode.hpp>

#include <iostream>

namespace WORLD
{

SceneNodeManipulator::SceneNodeManipulator()
    : m_FocusedNode{ nullptr }
    , m_PlaneAxis{ Axis(0) }
    , m_DraggedAxis{ Axis(0) }
    , m_BeginTranslatePoint{ 0.0f, 0.0f, 0.0f }
    , m_BeginNodePosition{ 0.0f, 0.0f, 0.0f }
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

glm::vec3 SceneNodeManipulator::AxisToVector(SceneNodeManipulator::Axis axis)
{
    switch (axis)
    {
    case SceneNodeManipulator::Axis::X:
        return glm::vec3{ 1.0f, 0.0f, 0.0f };
    case SceneNodeManipulator::Axis::Y:
        return glm::vec3{ 0.0f, 1.0f, 0.0f };
    case SceneNodeManipulator::Axis::Z:
        return glm::vec3{ 0.0f, 0.0f, 1.0f };
    default:
        DRE_ASSERT(false, "Invalid dragging plane normal.");
        return glm::vec3{ 0.0f, 0.0f, 0.0f };
    }
}

bool SceneNodeManipulator::TryInteract(SYS::InputSystem& inputSystem, GFX::RenderView& view)
{
    if (m_FocusedNode == nullptr)
        return false;

    bool hitGizmo = false;

    glm::ivec2 mousePos{ inputSystem.GetMouseState().mousePosX_, inputSystem.GetMouseState().mousePosY_ };
    DRE::Ray cameraRay = DRE::RayFromCamera(mousePos, view.GetSize(), view.GetInvViewProjectionM(), view.GetPosition());

    if (inputSystem.GetLeftMouseButtonJustPressed())
    {
        m_DraggedAxis = Axis(0);

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

        if (m_DraggedAxis != Axis(0))
        {
            m_BeginTranslatePoint = cameraRay.Evaluate(tMin);
            m_BeginNodePosition = m_FocusedNode->GetPosition();
        }
    }

    if (m_DraggedAxis != Axis(0) && inputSystem.GetLeftMouseButtonPressed())
    {
        hitGizmo = true;
        // DRAGGING: IMPLEMENT ME

        std::cout << "Interacted Axis: " << int(m_DraggedAxis) << std::endl;

        Axis axis = PickBestPlaneAxis(view.GetDirection());
        glm::vec3 dragPlaneNormal = AxisToVector(axis);
        DRE::Plane dragPlane = DRE::PlaneFromNormalAndPoint(dragPlaneNormal, m_BeginTranslatePoint);

        float t = 0.0f;
        if (DRE::RayPlaneIntersection(cameraRay, dragPlane, t))
        {
            glm::vec3 dragPoint = cameraRay.Evaluate(t);
            glm::vec3 dragAxis = AxisToVector(m_DraggedAxis);
            float diff = glm::dot(dragPoint, dragAxis);
            glm::vec3 targetPosition = m_BeginNodePosition + dragAxis * diff;
            std::cout << "DIFF: " << diff << std::endl;
            m_FocusedNode->SetPosition(targetPosition);
        }

        if (inputSystem.GetLeftMouseButtonJustReleased())
            m_DraggedAxis = Axis(0); // stop dragging
    }

    return hitGizmo;
}

}

