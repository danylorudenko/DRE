#pragma once

#include <foundation\Common.hpp>

namespace GFX
{

const float C_EV100_BASE_LUMINANCE = 0.125f; // 0 EV = 0.125 cd/m^2 (nits)
const float C_EV100_DEFAULT_TARGET = 16.0f; // luminance of white matte object under sunlight

float CalculateExposureCompensation(float targetEV = C_EV100_DEFAULT_TARGET);


} // namespace GFX
