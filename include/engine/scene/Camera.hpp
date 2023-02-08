#pragma once

#include <glm\vec3.hpp>
#include <glm\mat4x4.hpp>

namespace WORLD
{

class Camera
{
public:
    Camera();

    inline glm::vec3 const& GetPosition() const { return m_Position; }
    inline glm::vec3 const& GetEulerOrientation() const { return m_EulerOrientation; }
    inline glm::vec3 const& GetDirection() const { return m_Direction; }
    inline float            GetFOV() const { return m_FOV; }

    inline glm::mat4 const& GetViewM() const { return m_ViewM; }
    inline glm::mat4 const& GetProjM() const { return m_ProjM; }

    void                    Move(glm::vec3 dir);
    void                    Rotate(glm::vec3 euler);

    void                    SetPosition(glm::vec3 p);
    void                    SetEulerOrientation(glm::vec3 o);
    void                    SetFOV(float fov);
    void                    SetAspect(float aspect);

private:
    void                    CalculateView();
    void                    CalculateProjectionMatrix();

private:
    glm::vec3   m_Position;
    glm::vec3   m_EulerOrientation;
    glm::vec3   m_Direction;
    float       m_FOV;
    float       m_Aspect;

    glm::mat4   m_ViewM;
    glm::mat4   m_ProjM;
};

}

