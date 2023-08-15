#include <engine\scene\SceneNode.hpp>

#include <foundation\memory\Memory.hpp>
#include <gfx\GraphicsManager.hpp>
#include <engine\data\Material.hpp>


namespace WORLD
{

SceneNode::SceneNode()
    : m_Parent{ nullptr }
    , m_Position{ 0.0f, 0.0f, 0.0f }
    , m_Orientation{ 0.0f, 0.0f, 0.0f }
    , m_Forward{ 0.0f, 0.0f, -1.0f }
    , m_Right{ 1.0f, 0.0f, 0.0f }
    , m_Up{ 0.0f, 1.0f, 0.0f }
    , m_Scale{ 1.0f }
    , m_Children{ &DRE::g_MainAllocator }
{}

SceneNode::SceneNode(SceneNode* parent)
    : m_Parent{ parent }
    , m_Position{ 0.0f, 0.0f, 0.0f }
    , m_Orientation{ 0.0f, 0.0f, 0.0f }
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

glm::vec3 SceneNode::GetGlobalOrientation() const
{
    return m_Parent != nullptr ? m_Parent->GetGlobalOrientation() + m_Orientation : m_Orientation;
}

float SceneNode::GetGlobalScale() const
{
    return m_Parent != nullptr ? m_Parent->GetGlobalScale() * m_Scale : m_Scale;
}

void SceneNode::SetParent(SceneNode* parent)
{
    m_Parent = parent;
}

glm::mat4 SceneNode::GetGlobalMatrix() const
{
    glm::mat4x4 matrix{
        m_Scale, 0.0f, 0.0f, 0.0f,
        0.0f, m_Scale, 0.0f, 0.0f,
        0.0f, 0.0f, m_Scale, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    glm::vec3 const euler = glm::radians(m_Orientation);

    matrix = glm::rotate(matrix, euler.y, glm::vec3{ 0.0f, 1.0f, 0.0f }); // yaw
    matrix = glm::rotate(matrix, euler.x, glm::vec3{ 1.0f, 0.0f, 0.0f }); // pitch
    matrix = glm::rotate(matrix, euler.z, glm::vec3{ 0.0f, 0.0f, 1.0f }); // roll

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

    glm::vec3 const euler = glm::radians(m_Orientation);

    matrix = glm::rotate(matrix, euler.y, glm::vec3{ 0.0f, 1.0f, 0.0f }); // yaw
    matrix = glm::rotate(matrix, euler.x, glm::vec3{ 1.0f, 0.0f, 0.0f }); // pitch
    matrix = glm::rotate(matrix, euler.z, glm::vec3{ 0.0f, 0.0f, 1.0f }); // roll

    matrix[3][0] = m_Position.x;
    matrix[3][1] = m_Position.y;
    matrix[3][2] = m_Position.z;

    return m_Parent ? m_Parent->GetGlobalMatrix() * matrix : matrix;
}

void SceneNode::SetMatrix(glm::mat4 const& matrix)
{
    I STOPPED HERE
}

void SceneNode::CalculateDirectionVectors()
{
    glm::vec3 const euler = glm::radians(GetGlobalOrientation());

    glm::mat4 rotationM = glm::identity<glm::mat4>();
    rotationM = glm::rotate(rotationM, euler.y, glm::vec3{ 0.0f, 1.0f, 0.0f }); // yaw
    rotationM = glm::rotate(rotationM, euler.x, glm::vec3{ 1.0f, 0.0f, 0.0f }); // pitch
    rotationM = glm::rotate(rotationM, euler.z, glm::vec3{ 0.0f, 0.0f, 1.0f }); // roll

    m_Forward = rotationM * glm::vec4{ 0.0f, 0.0f, -1.0f, 0.0f };
    m_Right = rotationM * glm::vec4{ 1.0f, 0.0f, 0.0f, 0.0f };
    m_Up = glm::cross(m_Right, m_Forward);
}

}

