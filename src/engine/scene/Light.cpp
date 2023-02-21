#include <engine\scene\Light.hpp>

#include <glm\trigonometric.hpp>
#include <glm\gtc\matrix_transform.hpp>

namespace WORLD
{

DirectionalLight::DirectionalLight()
    : Positional{}
    , m_Radiance{ 1.0f, 1.0f, 1.0f }
{}

}

