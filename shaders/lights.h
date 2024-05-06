#ifndef _LIGHTS_H_
#define _LIGHTS_H_

#include "shaders_defines.h"

#define DRE_LIGHT_TYPE_SUN          0
#define DRE_LIGHT_TYPE_DIRECTIONAL  1
#define DRE_LIGHT_TYPE_POINT        2
#define DRE_LIGHT_TYPE_MAX          3

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
    return light.direction_type.xyz;
}

uint GetType(S_LIGHT_GPURef light)
{
    return floatBitsToUint(light.direction_type.w);
}

vec3 GetSpectrum(S_LIGHT_GPURef light)
{
    return light.spectrum_flux.rgb;
}

float GetFlux(S_LIGHT_GPURef light)
{
    return light.spectrum_flux.w;
}

#endif // !__cplusplus

#endif // _LIGHTS_H_