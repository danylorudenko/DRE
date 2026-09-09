#include <engine\scene\Light.hpp>

#include <glm\trigonometric.hpp>
#include <glm\gtc\matrix_transform.hpp>

namespace WORLD
{

Light::Light(GFX::LightsManager* lightsManager, DRE::U32 type)
    : ISceneNodeUser{ nullptr, ISceneNodeUser::Type::Light }
    , m_Type{ type }
    , m_Spectrum{ 1.0f, 1.0f, 1.0f }
    , m_Intensity{ 1.0f }
    , m_Radius{ 1.0f }
    , m_Falloff{ 1.0f }
    , m_GPULight{ lightsManager->AllocateLight() }
{
}

double Light::GetFlux() // lumens (lm) for point and spot lights
{
    DRE_ASSERT(m_Type != DRE_LIGHT_TYPE_DIRECTIONAL, "Lighting unit inconsistent with light type.");
    return m_Intensity * 4.0 * glm::pi<double>();
}

double Light::GetLuminance() // for spot lights cd (lm/sr)
{
    DRE_ASSERT(m_Type != DRE_LIGHT_TYPE_DIRECTIONAL, "Lighting unit inconsistent with light type.");
    return m_Intensity;
}

double Light::GetIllumiance() // for directional lights nits (cd/m2)
{
    DRE_ASSERT(m_Type == DRE_LIGHT_TYPE_DIRECTIONAL, "Lighting unit inconsistent with light type.");
    return m_Intensity;
}

void Light::SetFluxLumen(double flux)
{
    DRE_ASSERT(m_Type != DRE_LIGHT_TYPE_DIRECTIONAL, "Lighting unit inconsistent with light type.");
    m_Intensity = flux / (4.0 * glm::pi<double>());
}

void Light::SetLuminanceCd(double luminance) // for spot lights cd (lm/sr)
{
    DRE_ASSERT(m_Type != DRE_LIGHT_TYPE_DIRECTIONAL, "Lighting unit inconsistent with light type.");
    m_Intensity = luminance;
}

void Light::SetIlluminanceLux(double illuminance) // for directional lights lux (lumen/m^2)
{
    DRE_ASSERT(m_Type == DRE_LIGHT_TYPE_DIRECTIONAL, "Lighting unit inconsistent with light type.");
    m_Intensity = illuminance;
}

void Light::SetLightType(DRE::U32 type)
{
    DRE_ASSERT(false, "Not implemented properly yet. Need to convert the intesity properly");
    m_Type = type;
}

void Light::OnTransformChanged(glm::mat4 const& transform)
{
    ScheduleUpdateGPUData();
}

void Light::ScheduleUpdateGPUData()
{
    m_GPULight.ScheduleUpdate(GetGlobalPosition(), -GetForward(), GetSpectrum(), m_Intensity, m_Type);
}

}

