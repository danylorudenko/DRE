#version 450 core

#extension GL_GOOGLE_include_directive : enable

#define ENABLE_PCF
#define ENABLE_PCF_POISSON

#include "shaders_common.h"
#include "lighting.h"
#include "shadows.h"

layout(location = 0) in vec3 in_wpos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_prev_wpos;
layout(location = 3) in mat3 in_TBN;

layout(location = 0) out vec4 finalColor;
layout(location = 1) out vec2 velocity;

layout(set = 3, binding = 0) uniform texture2D shadowMap;
layout(set = 3, binding = 1, std140) uniform PassUniform
{
    mat4  shadow_VP;
    vec4  shadow_size;
} passUniform;
layout(set = 3, binding = 2) uniform texture2D causticMap;

layout(set = 4, binding = 0, std140) uniform InstanceUniform
{
    mat4  model_mat;
    mat4  prev_model_mat;
    uvec4 textureIDs;
} instanceUniform;



#define DiffuseTextureID     instanceUniform.textureIDs[0]
#define NormalTextureID      instanceUniform.textureIDs[1]
#define MetalnessTextureID   instanceUniform.textureIDs[2]
#define RoughnessTextureID   instanceUniform.textureIDs[3]

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

}
