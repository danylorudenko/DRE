#pragma once

#include <glm\vec3.hpp>

#include <engine\scene\ISceneNodeUser.hpp>

namespace WORLD
{

class DirectionalLight : public ISceneNodeUser
{
public:
    DirectionalLight();

    inline glm::vec3 const& GetDirection() { return m_SceneNode->GetForward(); }

    inline glm::vec3 const& GetRadiance() { return m_Radiance; }
    inline void             SetRadiance(glm::vec3 const& r) { m_Radiance = r; }

private:
    glm::vec3   m_Radiance;

};

}

