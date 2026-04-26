#ifndef _DDGI_COMMON_H_
#define _DDGI_COMMON_H_

#include "common/shaders_defines.h"

struct DDGIVolumeDesc
{
    float3  probeGridOrigin;
    float   probeSpacing;

    uint3   probeGridDimensions;
    uint    raysPerProbe;

    uint    irradianceProbeSize;
    uint    visibilityProbeSize;
    float   maxProbeRayDistance;
    float   depthSharpness;

    float   hysteresis;
    float   irradianceThreshold;
    float   brightnessThreshold;
    float   _pad0;
};

#endif // _DDGI_COMMON_H_