#include <engine\scene\Camera.hpp>

#include <glm\trigonometric.hpp>
#include <glm\gtc\matrix_transform.hpp>
namespace WORLD
{

Camera::Camera()
    : Positional{}
    , m_FOV{ 60.0f }
{
}

void Camera::CalculateVectors()
{
    glm::vec3 const euler = glm::radians(m_EulerOrientation);

    glm::mat4 rotationM = glm::identity<glm::mat4>();
    rotationM = glm::rotate(rotationM, euler.y, glm::vec3{ 0.0f, 1.0f, 0.0f }); // yaw
    rotationM = glm::rotate(rotationM, euler.x, glm::vec3{ 1.0f, 0.0f, 0.0f }); // pitch
    rotationM = glm::rotate(rotationM, euler.z, glm::vec3{ 0.0f, 0.0f, 1.0f }); // roll

    m_Forward = rotationM * glm::vec4{ 0.0f, 0.0f, -1.0f, 0.0f };
    m_Up = glm::vec3{ 0.0f, 1.0f, 0.0f };
    m_Right = glm::cross(m_Forward, m_Up);
}

void Camera::SetFOV(float fov)
{
    m_FOV = fov;
}

}

