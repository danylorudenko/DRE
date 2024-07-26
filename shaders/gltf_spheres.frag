#version 450 core

#extension GL_GOOGLE_include_directive : enable

#define ENABLE_PCF
#define ENABLE_PCF_POISSON

#include "shaders_common.h"
#include "lighting.h"
#include "shadows.h"
#include "forward.h"
#include "forward_output.h"

layout(location = 0) in vec3 in_wpos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_prev_wpos;
layout(location = 3) in mat3 in_TBN;

void main()
{
    vec3 diffuse    = SampleGlobalTextureAnisotropic(DiffuseTextureID, in_uv).rgb;
    vec3 normal     = SampleGlobalTextureAnisotropic(NormalTextureID, in_uv).rgb;
    vec2 metalness_roughness = SampleGlobalTextureAnisotropic(MetalnessTextureID, in_uv).bg;
    float metalness = metalness_roughness.x;
    float roughness = metalness_roughness.y;

    S_SURFACE surface;
    surface.wpos = in_wpos;
    surface.normal = normalize(in_TBN[2]);
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
