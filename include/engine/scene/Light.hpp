#pragma once

#include <glm\vec3.hpp>

#include <engine\scene\SceneNode.hpp>

namespace WORLD
{

class DirectionalLight
{
public:
    DirectionalLight(SceneNode* node = nullptr);

    inline glm::vec3 const& GetDirection() { return m_SceneNode->GetForward(); }

    inline glm::vec3 const& GetRadiance() { return m_Radiance; }
    inline void             SetRadiance(glm::vec3 const& r) { m_Radiance = r; }

    inline void             SetSceneNode(SceneNode* node) { m_SceneNode = node; }
    inline SceneNode*       GetSceneNode() const { return m_SceneNode; }

private:
    glm::vec3   m_Radiance;
    SceneNode*  m_SceneNode;

};

}

