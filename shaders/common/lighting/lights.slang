#ifndef _LIGHTS_H_
#define _LIGHTS_H_

#include "common/shaders_defines.slang"

#define DRE_LIGHT_TYPE_SUN          0
#define DRE_LIGHT_TYPE_DIRECTIONAL  1
#define DRE_LIGHT_TYPE_POINT        2
#define DRE_LIGHT_TYPE_MAX          3

struct S_LIGHT
{
    float4 world_pos;
    float4 direction_type;
    float4 spectrum_flux;
};

#ifndef __cplusplus

float3 GetWorldPos(S_LIGHT* light)
{
    return light.world_pos.xyz;
}

float3 GetDirection(S_LIGHT* light)
{
    return light.direction_type.xyz;
}

uint GetType(S_LIGHT* light)
{
    return asuint(light.direction_type.w);
}

float3 GetSpectrum(S_LIGHT* light)
{
    return light.spectrum_flux.rgb;
}

float GetFlux(S_LIGHT* light)
{
    return light.spectrum_flux.w;
}

#endif // !__cplusplus

#endif // _LIGHTS_H_
