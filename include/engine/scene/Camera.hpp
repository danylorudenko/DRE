#pragma once

#include <glm\vec3.hpp>
#include <glm\vec2.hpp>
#include <glm\mat4x4.hpp>

namespace WORLD
{

class Camera
{
public:
    Camera();

    inline glm::vec3 const& GetPosition() const { return m_Position; }
    inline glm::vec3 const& GetEulerOrientation() const { return m_EulerOrientation; }
    inline glm::vec3 const& GetForward() const { return m_Forward; }
    inline glm::vec3 const& GetRight() const { return m_Right; }
    inline glm::vec3        GetUp() const { return glm::vec3{ 0.0f, 1.0f, 0.0f }; }
    inline glm::vec2        GetRange() const { return glm::vec2{ 0.01f, 1000.0f }; }

    inline float            GetFOV() const { return m_FOV; }

    void                    Move(glm::vec3 dir);
    void                    Rotate(glm::vec3 euler);

    void                    SetPosition(glm::vec3 p);
    void                    SetEulerOrientation(glm::vec3 o);
    void                    SetFOV(float fov);

private:
    void                    CalculateVectors();

private:
    glm::vec3   m_Position;
    glm::vec3   m_EulerOrientation;
    glm::vec3   m_Forward;
    glm::vec3   m_Right;
    float       m_FOV;
};

}

