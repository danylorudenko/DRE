#include <engine\scene\SceneNode.hpp>

#include <foundation\memory\Memory.hpp>
#include <gfx\GraphicsManager.hpp>
#include <engine\data\Material.hpp>


namespace WORLD
{

SceneNode::SceneNode()
    : m_Parent{ nullptr }
    , m_Position{ 0.0f, 0.0f, 0.0f }
    , m_Orientation{ glm::identity<glm::quat>() }
    , m_Forward{ 0.0f, 0.0f, -1.0f }
    , m_Right{ 1.0f, 0.0f, 0.0f }
    , m_Up{ 0.0f, 1.0f, 0.0f }
    , m_Scale{ 1.0f }
    , m_Children{ &DRE::g_MainAllocator }
{}

SceneNode::SceneNode(SceneNode* parent)
    : m_Parent{ parent }
    , m_Position{ 0.0f, 0.0f, 0.0f }
    , m_Orientation{ glm::identity<glm::quat>() }
    , m_Forward{ 0.0f, 0.0f, -1.0f }
    , m_Right{ 1.0f, 0.0f, 0.0f }
    , m_Up{ 0.0f, 1.0f, 0.0f }
    , m_Scale{ 1.0f }
    , m_Children{ &DRE::g_MainAllocator }
{}

void SceneNode::AddChild(SceneNode* child)
{
#ifdef DRE_DEBUG
    const DRE::U32 id = m_Children.Find(child);
    DRE_ASSERT(id == m_Children.Size(), "Adding duplicate child to SceneNode");
#endif // DRE_DEBUG

    m_Children.EmplaceBack(child);
}

void SceneNode::RemoveChild(SceneNode* child)
{
    const DRE::U32 i = m_Children.Find(child);
    DRE_ASSERT(i != m_Children.Size(), "Removing invalid child from SceneNode");
    m_Children.RemoveIndex(i);
}

glm::vec3 SceneNode::GetGlobalPosition() const
{
    return m_Parent != nullptr ? m_Parent->GetGlobalPosition() + m_Position : m_Position;
}

glm::quat SceneNode::GetGlobalOrientation() const
{
    return m_Parent != nullptr ? m_Parent->GetGlobalOrientation() * m_Orientation : m_Orientation;
}

glm::vec3 SceneNode::GetGlobalEulerOrientation() const
{
    return m_Parent != nullptr ? m_Parent->GetGlobalEulerOrientation() + GetEulerOrientation() : GetEulerOrientation();
}

float SceneNode::GetGlobalScale() const
{
    return m_Parent != nullptr ? m_Parent->GetGlobalScale() * m_Scale : m_Scale;
}

void SceneNode::SetParent(SceneNode* parent)
{
    m_Parent = parent;
}

glm::vec3 SceneNode::GetEulerOrientation() const
{
    return glm::degrees(glm::eulerAngles(m_Orientation));
}

glm::mat4 SceneNode::GetGlobalMatrix() const
{
    glm::mat4x4 matrix{
        m_Scale, 0.0f, 0.0f, 0.0f,
        0.0f, m_Scale, 0.0f, 0.0f,
        0.0f, 0.0f, m_Scale, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    matrix = glm::mat4{ m_Orientation } * matrix;

    matrix[3][0] = m_Position.x;
    matrix[3][1] = m_Position.y;
    matrix[3][2] = m_Position.z;

    return m_Parent ? m_Parent->GetGlobalMatrix() * matrix : matrix;
}

glm::mat4 SceneNode::GetGlobalMatrixNoScale() const
{
    glm::mat4x4 matrix{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    matrix = glm::mat4{ m_Orientation } *matrix;

    matrix[3][0] = m_Position.x;
    matrix[3][1] = m_Position.y;
    matrix[3][2] = m_Position.z;

    return m_Parent ? m_Parent->GetGlobalMatrix() * matrix : matrix;
}

void SceneNode::SetMatrix(glm::mat4 const& matrix)
{
    m_Position[0] = matrix[3][0];
    m_Position[1] = matrix[3][1];
    m_Position[2] = matrix[3][2];

    glm::mat3 mat = glm::mat3(matrix);
    glm::vec3 scale = glm::vec3{ glm::length(mat[0]), glm::length(mat[1]), glm::length(mat[2]) };

    mat[0] /= scale[0];
    mat[1] /= scale[1];
    mat[2] /= scale[2];

    m_Orientation = glm::quat{ mat };
}

void SceneNode::SetOrientation(glm::quat const& orientation)
{
    m_Orientation = orientation;
    CalculateDirectionVectors();
}

void SceneNode::SetEulerOrientation(glm::vec3 const& orientation)
{
    m_Orientation = glm::quat{ glm::radians(orientation) };
    CalculateDirectionVectors();
}

void SceneNode::Rotate(glm::quat const& rotation)
{
    m_Orientation *= rotation;
    CalculateDirectionVectors();
}

void SceneNode::Rotate(glm::vec3 const& eulerRotation)
{
    m_Orientation *= glm::quat{ glm::radians(eulerRotation) };
    CalculateDirectionVectors();
}

void SceneNode::CalculateDirectionVectors()
{
    glm::quat orientation = GetGlobalOrientation();

    glm::mat4 rotationM = glm::mat4_cast(orientation);

    m_Forward = rotationM * glm::vec4{ 0.0f, 0.0f, -1.0f, 0.0f };
    m_Right = rotationM * glm::vec4{ 1.0f, 0.0f, 0.0f, 0.0f };
    m_Up = glm::cross(m_Right, m_Forward);
}

}

