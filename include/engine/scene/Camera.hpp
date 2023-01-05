#pragma once

#include <glm\vec3.hpp>
#include <glm\trigonometric.hpp>

namespace WORLD
{

class Camera
{
public:
    Camera();

    inline glm::vec3 const& GetPosition() const { return m_Position; }
    inline glm::vec3 const& GetEulerOrientation() const { return m_EulerOrientation; }
    inline float            GetFOV() const { return m_FOV; }

    inline void             Move(glm::vec3 m) { m_Position += m; }
    inline void             Rotate(glm::vec3 v) { m_EulerOrientation += v; }

    inline void             SetPosition(glm::vec3 p) { m_Position = p; }
    inline void             SetEulerOrientation(glm::vec3 o) { m_EulerOrientation = o; }
    inline void             SetFOV(float fov) { m_FOV = fov; }

private:
    glm::vec3   m_Position;
    glm::vec3   m_EulerOrientation;
    float       m_FOV;
};

}

