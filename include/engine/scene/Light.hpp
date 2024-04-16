#pragma once

#include <glm\vec3.hpp>

#include <engine\scene\ISceneNodeUser.hpp>
#include <gfx\renderer\LightsManager.hpp>

namespace GFX
{

class LightsManager;

}

namespace WORLD
{

class Light : public ISceneNodeUser
{
public:
    Light(GFX::LightsManager* lightsManager);

    inline glm::vec3 const& GetDirection() { return m_SceneNode->GetForward(); }

    inline float GetRadiance() { return m_Radiance; }
    inline void  SetRadiance(float r) { m_Radiance = r; }

    inline glm::vec3 const& GetSpectrum() const { return m_Spectrum; }
    inline void SetSpectrum(glm::vec3 const& s) { m_Spectrum = s; }

private:
    glm::vec3                   m_Spectrum;
    float                       m_Radiance;
    GFX::LightsManager::Light   m_GPULight;
};

}

