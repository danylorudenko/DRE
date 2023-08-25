#pragma once

#include <glm\vec2.hpp>

#include <engine\scene\ISceneNodeUser.hpp>

namespace WORLD
{

class Camera : public ISceneNodeUser
{
public:
    Camera(SceneNode* node = nullptr);

    inline glm::vec2        GetRange() const { return glm::vec2{ 0.1f, 1000.0f }; }
    inline float            GetFOV() const { return m_FOV; }
    void                    SetFOV(float fov);

    void                    SetCameraEuler(glm::vec3 euler);
    glm::vec3 const&        GetCameraEuler(glm::vec3 euler) const;

    void                    RotateCamera(glm::vec3 euler);

private:
    glm::vec3   m_CameraEuler;
    float       m_FOV;
};

}

