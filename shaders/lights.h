#ifndef __LIGHTS_H__
#define __LIGHTS_H__

#include "shaders_defines.h"

DeclareStorageBuffer(S_LIGHT)
{
    vec4 world_pos;
    vec4 direction;
    vec4 radiant_flux_spectrum;
};

#endif // __LIGHTS_H__