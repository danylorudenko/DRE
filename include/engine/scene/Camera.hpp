#pragma once

#include <glm\vec2.hpp>

#include <engine\scene\SceneNode.hpp>

namespace WORLD
{

class Camera
{
public:
    Camera(SceneNode* node = nullptr);

    inline glm::vec2        GetRange() const { return glm::vec2{ 0.1f, 100.0f }; }
    inline float            GetFOV() const { return m_FOV; }
    void                    SetFOV(float fov);

    inline void             SetSceneNode(SceneNode* node) { m_SceneNode = node; }
    inline SceneNode*       GetSceneNode() const { return m_SceneNode; }


private:
    float       m_FOV;
    SceneNode*  m_SceneNode;
};

}

