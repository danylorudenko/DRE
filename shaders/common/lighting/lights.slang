#ifndef _LIGHTS_H_
#define _LIGHTS_H_

#include "common/shaders_defines.slang"

#define DRE_LIGHT_TYPE_DIRECTIONAL  0
#define DRE_LIGHT_TYPE_POINT        1
#define DRE_LIGHT_TYPE_SPOT         2
#define DRE_LIGHT_TYPE_MAX          3

struct S_LIGHT
{
    float4 world_pos;
    float4 direction_type;
    // xyz - spectrum, w - intensity
    // intensity is in lux for directional lights
    // local (spot/point) in candelas (cd)
    float4 spectrum_intensity;
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
    return light.spectrum_intensity.rgb;
}

float GetIntensity(S_LIGHT* light)
{
    return light.spectrum_intensity.w;
}


#endif // !__cplusplus

#endif // _LIGHTS_H_
