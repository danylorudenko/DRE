#version 460 core

#extension GL_GOOGLE_include_directive : enable

#define ENABLE_PCF
#define ENABLE_PCF_POISSON

#include "common/shaders_common.h"
#include "common/lighting/lighting.h"
#include "common/forward.h"
#include "common/forward_output.h"

layout(location = 0) in vec3 in_wpos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_prev_wpos;
layout(location = 3) in mat3 in_TBN;

void main()
{
    S_INSTANCE_GPURef InstanceRef = GetInstance();

    vec3 diffuse    = SampleGlobalTextureAnisotropic(GetDiffuseTextureID(InstanceRef), in_uv).rgb;
    vec3 normal     = SampleGlobalTextureAnisotropic(GetNormalTextureID(InstanceRef), in_uv).rgb;
    float metalness = SampleGlobalTextureAnisotropic(GetMetalnessTextureID(InstanceRef), in_uv).r;
    float roughness = SampleGlobalTextureAnisotropic(GetRoughnessTextureID(InstanceRef), in_uv).r;

    vec3 n = vec3(0,0,1);
    uint normalMode = GetNormalMode(InstanceRef);
    switch(normalMode)
    {
        case NORMAL_MODE_TEXTURE:
            n = normalize(in_TBN * (normal * 2.0 - 1.0));
            break;
        case NORMAL_MODE_TEXTURE_INVERT_Y:
            normal = vec3(normal.r, 1 - normal.g, normal.b);
            n = normalize(in_TBN * (normal * 2.0 - 1.0));
            break;
        case NORMAL_MODE_TBN:
            n = normalize(in_TBN[2]);
            break;
    }

    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(
        rayQuery,
        g_TLAS,
        gl_RayFlagsTerminateOnFirstHitEXT,
        0xFFFFFFFF,
        in_wpos,
        0.1f, // tMin
        normalize(vec3(1.0f,  10.0f, 1.0f)),
        10000); // tMax

    rayQueryProceedEXT(rayQuery);

    if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT)
    {
        diffuse *= vec3(0.1f, 0.1f, 0.1f);
    }

    S_SURFACE surface;
    surface.wpos = in_wpos;
    surface.normal = n;
    surface.diffuseSpectrum = diffuse;
    surface.roughness = roughness;
    surface.metalness = metalness;
    surface.prevWpos = in_prev_wpos;

    S_LIGHTING_RESULT lighting = CalculateLighting(surface);

    OutputForwardPass(lighting, surface);
}
