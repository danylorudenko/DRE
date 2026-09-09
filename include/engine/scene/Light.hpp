#pragma once

#include <glm\vec3.hpp>

#include <engine\scene\ISceneNodeUser.hpp>
#include <gfx\renderer\GPULightsManager.hpp>

#include <common\lighting\lights.slang>

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
        Light(GFX::LightsManager* lightsManager, DRE::U32 type);

    inline glm::vec3 const& GetDirection() { return m_SceneNode->GetForward(); }

    double GetFlux();
    double GetIllumiance(); // for directional lights cd/m2
    double GetLuminance(); // for spot lights cd (lm/sr)

    void SetFluxLumen(double flux);
    void SetIlluminanceLux(double illuminance); // for directional lights lux (lumen/m^2)
    void SetLuminanceCd(double luminance); // for spot lights cd (lm/sr)


    inline glm::vec3 const& GetSpectrum() const { return m_Spectrum; }
    inline void SetSpectrum(glm::vec3 const& s) { m_Spectrum = s; }

    // DRE_LIGHT_TYPE_
    inline DRE::U32 GetLightType() const { return m_Type; }
    void SetLightType(DRE::U32 type/*DRE_LIGHT_TYPE_*/);

    void    ScheduleUpdateGPUData();
    virtual void OnTransformChanged(glm::mat4 const& transform) override;

private:
    DRE::U32                        m_Type = DRE_LIGHT_TYPE_MAX; // DRE_LIGHT_TYPE_
    glm::vec3                       m_Spectrum;
    double                          m_Intensity; // candelas for local lights, illuminance for directional lights
    float                           m_Radius;
    float                           m_Falloff;

    GFX::LightsManager::LightGPU    m_GPULight;
};

}

