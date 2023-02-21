#include <engine\scene\Positional.hpp>

#include <glm\trigonometric.hpp>
#include <glm\gtc\matrix_transform.hpp>

namespace WORLD
{

Positional::Positional()
    : m_Position{ 0.0f, 0.0f, 0.0f }
    , m_EulerOrientation{ 0.0f, 0.0f, 0.0f }
    , m_Forward{ 0.0f, 0.0f, -1.0f }
    , m_Right{ 1.0f, 0.0f, 0.0f }
    , m_Up{ 0.0f, 1.0f, 0.0f }
{
    CalculateVectors();
}

void Positional::CalculateVectors()
{
    glm::vec3 const euler = glm::radians(m_EulerOrientation);

    glm::mat4 rotationM = glm::identity<glm::mat4>();
    rotationM = glm::rotate(rotationM, euler.y, glm::vec3{ 0.0f, 1.0f, 0.0f }); // yaw
    rotationM = glm::rotate(rotationM, euler.x, glm::vec3{ 1.0f, 0.0f, 0.0f }); // pitch
    rotationM = glm::rotate(rotationM, euler.z, glm::vec3{ 0.0f, 0.0f, 1.0f }); // roll

    m_Forward = rotationM * glm::vec4{ 0.0f, 0.0f, -1.0f, 0.0f };
    m_Right = rotationM * glm::vec4{ 1.0f, 0.0f, 0.0f, 0.0f };
    m_Up = glm::cross(m_Right, m_Forward);
}

void Positional::SetPosition(glm::vec3 pos)
{
    m_Position = pos;
}

void Positional::SetEulerOrientation(glm::vec3 orientation)
{
    m_EulerOrientation = orientation;
    CalculateVectors();
}

void Positional::Move(glm::vec3 dir)
{
    m_Position += dir;
}

void Positional::Rotate(glm::vec3 euler)
{
    m_EulerOrientation += euler;
    CalculateVectors();
}

}

