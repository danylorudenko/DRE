#include <gfx\Photometric.hpp>

#include <glm\exponential.hpp>

namespace GFX
{

float CalculateExposureCompensation(float targetEV)
{
    return glm::exp2(-targetEV);
}

} // namespace GFX
