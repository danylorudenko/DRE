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

    vec3 n = normalize(in_TBN * (normal * 2.0 - 1.0));

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
        diffuse = vec3(0.0f, 0.0f, 0.0f);
    }

    S_SURFACE surface;
    surface.wpos = in_wpos;
    surface.normal = n;
    surface.diffuseSpectrum = diffuse;
    surface.roughness = roughness;
    surface.metalness = metalness;

    S_LIGHTING_RESULT lighting = CalculateLighting(surface);

    finalColor = vec4(lighting.finalRadiance, 1.0);

    vec4 prev_ndc = GetPrevCameraViewProjM() * in_prev_wpos;
    prev_ndc /= prev_ndc.w;

    vec2 pixel_pos_uv = gl_FragCoord.xy / GetViewportSize();
    vec2 pixel_pos_ndc = pixel_pos_uv * 2.0 - 1.0;

    vec2 vel = (pixel_pos_ndc - prev_ndc.xy);
    vec2 vel_uv = vel * 0.5;

    velocity = vec2(vel_uv);
    id = GlobalID2Color();
}
