#version 450 core

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#define ENABLE_PCF
#define ENABLE_PCF_POISSON

#include "shaders_common.h"
#include "shaders_defines.h"
#include "lighting_model.h"
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

    vec3 n = normalize(in_TBN * (normal * 2.0 - 1.0));
    vec3 v = normalize(GetCameraPos() - in_wpos);
    vec3 L = -GetMainLightDir();
    vec3 h = normalize(v + L);
    
    float NdotL = max(0.0, dot(n, L));
    float NdotH = max(0.0, dot(n, h));
    float NdotV = max(0.0, dot(n, v));
	
    vec3 brdf = CookTorranceBRDF(NdotH, NdotV, NdotL, diffuse, roughness, metalness);

    vec3 ambient = vec3(0.15, 0.15, 0.15) * diffuse;

    float shadow = CalculateShadow(in_wpos, passUniform.shadow_VP, passUniform.shadow_size.xy, shadowMap);

    vec3 res = shadow * brdf + ambient;    
	
    finalColor = vec4(res, 1.0);
	
	vec4 prev_ndc = GetPrevCameraViewProjM() * in_prev_wpos;
	prev_ndc /= prev_ndc.w;
	
	vec2 pixel_pos_uv = gl_FragCoord.xy / GetViewportSize();
	vec2 pixel_pos_ndc = pixel_pos_uv * 2.0 - 1.0;
	
	vec2 vel = (pixel_pos_ndc - prev_ndc.xy);
	vec2 vel_uv = vel * 0.5;
	
	velocity = vec2(vel_uv);
	
}
