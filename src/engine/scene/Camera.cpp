#include <engine\scene\Camera.hpp>

#include <glm\trigonometric.hpp>
#include <glm\gtc\matrix_transform.hpp>

namespace WORLD
{

Camera::Camera(SceneNode* node)
    : m_FOV{ 60.0f }
    , m_SceneNode{ node }
{
}

void Camera::SetFOV(float fov)
{
    m_FOV = fov;
}

}

