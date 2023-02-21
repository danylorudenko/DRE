#pragma once

#include <engine\scene\Positional.hpp>

namespace WORLD
{

class DirectionalLight : public Positional
{
public:
    DirectionalLight();

    inline glm::vec3 const& GetDirection() { return GetForward(); }

    inline glm::vec3 const& GetRadiance() { return m_Radiance; }
    inline void             SetRadiance(glm::vec3 const& r) { m_Radiance = r; }

private:
    glm::vec3 m_Radiance;

};

}

