#pragma once

#include <glm\vec3.hpp>

#include <engine\scene\ISceneNodeUser.hpp>
#include <gfx\renderer\LightsManager.hpp>

#include <lights.h>

namespace VKW
{
class Context;
}

namespace GFX
{
class LightsManager;
}

namespace WORLD
{

class Light : public ISceneNodeUser
{
public:
    Light(GFX::LightsManager* lightsManager, std::uint32_t type);

    inline glm::vec3 const& GetDirection() { return m_SceneNode->GetForward(); }

    inline float GetFlux() { return m_Flux; }
    inline void  SetFlux(float f) { m_Flux = f; }

    inline glm::vec3 const& GetSpectrum() const { return m_Spectrum; }
    inline void SetSpectrum(glm::vec3 const& s) { m_Spectrum = s; }

    void    ScheduleUpdateGPUData();

private:
    std::uint32_t               m_Type = DRE_LIGHT_TYPE_NONE; // DRE_LIGHT_TYPE_
    glm::vec3                   m_Spectrum;
    float                       m_Flux;
    float                       m_Radius;
    float                       m_Falloff;

    GFX::LightsManager::Light   m_GPULight;
};

}

