#include <engine\scene\Light.hpp>

#include <glm\trigonometric.hpp>
#include <glm\gtc\matrix_transform.hpp>

#include <gfx\renderer\LightsManager.hpp>

namespace WORLD
{

Light::Light(GFX::LightsManager* lightsManager)
    : ISceneNodeUser{ nullptr, ISceneNodeUser::Type::Light }
    , m_Spectrum{ 1.0f, 1.0f, 1.0f }
    , m_Radiance{ 1.0f }
    , m_GPULight{ lightsManager->AllocateLight() }
{}

}

