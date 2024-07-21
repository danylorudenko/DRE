#version 450 core

#extension GL_GOOGLE_include_directive : enable

#define ENABLE_PCF
#define ENABLE_PCF_POISSON

#include "shaders_common.h"
#include "lighting.h"
#include "forward.h"

layout(location = 0) in vec3 in_wpos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_prev_wpos;
layout(location = 3) in mat3 in_TBN;

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
    float metalness = SampleGlobalTextureAnisotropic(MetalnessTextureID, in_uv).r;
    float roughness = SampleGlobalTextureAnisotropic(RoughnessTextureID, in_uv).r;
	
	normal = vec3(normal.r, 1 - normal.g, normal.b);

    vec3 n = normalize(in_TBN * (normal * 2.0 - 1.0));
    vec3 v = normalize(GetCameraPos() - in_wpos);

    S_SURFACE surface;
    surface.wpos = in_wpos;
    surface.normal = n;
    surface.diffuseSpectrum = diffuse;
    surface.roughness = roughness;
    surface.metalness = metalness;

    S_LIGHTING_RESULT lighting = CalculateLighting(surface);

    finalColor = vec4(lighting.finalRadiance, 1.0);
	
    //finalColor = vec4(res, 1.0);
    //finalColor = vec4(in_TBN[2], 1.0);
    //finalColor = vec4(normal.rrr * 0.5 + 0.5, 1.0);
	
	vec4 prev_ndc = GetPrevCameraViewProjM() * in_prev_wpos;
	prev_ndc /= prev_ndc.w;
	
	vec2 pixel_pos_uv = gl_FragCoord.xy / GetViewportSize();
	vec2 pixel_pos_ndc = pixel_pos_uv * 2.0 - 1.0;
	
	vec2 vel = (pixel_pos_ndc - prev_ndc.xy);
	vec2 vel_uv = vel * 0.5;
	
	velocity = vec2(vel_uv);
	
}
