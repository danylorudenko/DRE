#include <engine\scene\Camera.hpp>

#include <glm\trigonometric.hpp>
#include <glm\gtc\matrix_transform.hpp>

namespace WORLD
{

Camera::Camera(SceneNode* node)
    : ISceneNodeUser{ node }
    , m_CameraEuler{ 0.0f, 0.0f, 0.0f }
    , m_FOV{ 60.0f }
{
}

void Camera::SetFOV(float fov)
{
    m_FOV = fov;
}

void Camera::SetCameraEuler(glm::vec3 euler)
{
    m_CameraEuler = euler;
    m_SceneNode->SetEulerOrientation(m_CameraEuler);
}

glm::vec3 const& Camera::GetCameraEuler(glm::vec3 euler) const
{
    return m_CameraEuler;
}

void Camera::RotateCamera(glm::vec3 euler)
{
    m_CameraEuler += euler;
    m_SceneNode->SetEulerOrientation(m_CameraEuler);
}

}

