#include <engine\scene\Light.hpp>

#include <glm\trigonometric.hpp>
#include <glm\gtc\matrix_transform.hpp>

namespace WORLD
{

Light::Light(GFX::LightsManager* lightsManager, std::uint32_t type)
    : ISceneNodeUser{ nullptr, ISceneNodeUser::Type::Light }
    , m_Type{ type }
    , m_Spectrum{ 1.0f, 1.0f, 1.0f }
    , m_Flux{ 1.0f }
    , m_Radius{ 1.0f }
    , m_Falloff{ 1.0f }
    , m_GPULight{ lightsManager->AllocateLight() }
{
}

void Light::ScheduleUpdateGPUData()
{
    m_GPULight.ScheduleUpdate(GetGlobalPosition(), -GetForward(), GetSpectrum(), GetFlux(), m_Type);
}

}

