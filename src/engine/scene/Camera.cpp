#include <engine\scene\Camera.hpp>

#include <glm\trigonometric.hpp>
#include <glm\gtc\matrix_transform.hpp>
namespace WORLD
{

Camera::Camera()
    : m_Position{ 0.0f, 0.0f, 0.0f }
    , m_EulerOrientation{ 0.0f, 0.0f, 0.0f }
    , m_Direction{ 0.0f, 0.0f, -1.0f }
    , m_FOV{ glm::radians(60.0f) }
    , m_Aspect{ 1.0f }
    , m_ViewM{ glm::identity<glm::mat4>() }
    , m_ProjM{ glm::identity<glm::mat4>() }
{

}

void Camera::CalculateView()
{
    glm::vec3 const up{ 0.0f, 1.0f, 0.0f };
    glm::vec3 const euler = glm::radians(m_EulerOrientation);

    glm::mat4 rotationM = glm::identity<glm::mat4>();
    rotationM = glm::rotate(rotationM, euler.y, glm::vec3{ 0.0f, 1.0f, 0.0f }); // yaw
    rotationM = glm::rotate(rotationM, euler.x, glm::vec3{ 1.0f, 0.0f, 0.0f }); // pitch
    rotationM = glm::rotate(rotationM, euler.z, glm::vec3{ 0.0f, 0.0f, 1.0f }); // roll

    m_Direction = rotationM * glm::vec4{ 0.0f, 0.0f, -1.0f, 0.0f };

    m_ViewM = glm::lookAtRH(m_Position, m_Direction, up);
}

void Camera::CalculateProjectionMatrix()
{
    m_ProjM = glm::perspectiveRH_ZO(glm::radians(m_FOV), m_Aspect, 0.1f, 1000.0f);
}

void Camera::SetPosition(glm::vec3 pos)
{
    m_Position = pos;
    CalculateView();
}

void Camera::SetEulerOrientation(glm::vec3 orientation)
{
    m_EulerOrientation = orientation;
    CalculateView();
}

void Camera::Move(glm::vec3 dir)
{
    m_Position += dir;
    CalculateView();
}

void Camera::Rotate(glm::vec3 euler)
{
    m_EulerOrientation += euler;
}

void Camera::SetFOV(float fov)
{
    m_FOV = fov;
    CalculateProjectionMatrix();
}

void Camera::SetAspect(float aspect)
{
    m_Aspect = aspect;
    CalculateProjectionMatrix();
}
}

