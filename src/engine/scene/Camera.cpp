#include <engine\scene\Camera.hpp>

namespace WORLD
{

Camera::Camera()
    : m_Position{ 0.0f, 0.0f, 0.0f }
    , m_EulerOrientation{ 0.0f, 0.0f, 0.0f }
    , m_FOV{ glm::radians(60.0f) }
{
}

}

