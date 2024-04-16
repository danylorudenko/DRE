#ifndef __LIGHTS_H__
#define __LIGHTS_H__

#include "shaders_defines.h"

#define DRE_LIGHT_TYPE_DIRECTIONAL  0
#define DRE_LIGHT_TYPE_POINT        1

DeclareStorageBuffer(S_LIGHT)
{
    vec4 world_pos;
    vec4 direction_type;
    vec4 spectrum_flux;
};

#ifndef __cplusplus

vec3 GetWorldPos(S_LIGHT_GPURef light)
{
    return light.world_pos.xyz;
}

vec3 GetDirection(S_LIGHT_GPURef light)
{
    return light.direction.xyz;
}

uint GetLightType(S_LIGHT_GPURef light)
{
    return floatBitsToUint(light.direction.w);
}

vec3 GetLightSpectrum(S_LIGHT_GPURef light)
{
    return light.spectrum_flux.rgb;
}

float GetLightFlux(S_LIGHT_GPURef light)
{
    return light.spectrum_flux.w;
}

#endif // !__cplusplus

#endif // __LIGHTS_H__