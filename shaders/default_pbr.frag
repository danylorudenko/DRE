#version 460 core

#extension GL_GOOGLE_include_directive : enable

#define ENABLE_PCF
#define ENABLE_PCF_POISSON

#include "common/shaders_common.h"
#include "common/lighting/lighting.h"
#include "common/forward.h"

layout(location = 0) in vec3 in_wpos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_prev_wpos;
layout(location = 3) in mat3 in_TBN;

void main()
{
    S_INSTANCE_GPURef InstanceRef = GetInstance();
    uint instanceFlags = GetInstanceFlags(InstanceRef);

    vec3 diffuse    = SampleGlobalTextureAnisotropic(GetDiffuseTextureID(InstanceRef), in_uv).rgb;
    vec3 normal     = SampleGlobalTextureAnisotropic(GetNormalTextureID(InstanceRef), in_uv).rgb;
    float metalness = 0.0;
    float roughness  = 0.0;

    if ((instanceFlags & INSTANCE_FLAG_MATERIAL_TEXTURES_DEFAULT) != 0)
    {
        metalness = SampleGlobalTextureAnisotropic(GetMetalnessTextureID(InstanceRef), in_uv).r;
        roughness = SampleGlobalTextureAnisotropic(GetRoughnessTextureID(InstanceRef), in_uv).r;
    }
    if ((instanceFlags & INSTANCE_FLAG_MATERIAL_TEXTURES_GLTF_SPHERES) != 0)
    {
        vec2 metalness_roughness = SampleGlobalTextureAnisotropic(GetMetalnessTextureID(InstanceRef), in_uv).bg;
        metalness = metalness_roughness.x;
        roughness = metalness_roughness.y;
    }

    vec3 n = vec3(0,0,1);
    if ((instanceFlags & INSTANCE_FLAG_NORMAL_TEXTURE) != 0)
    {
        n = normalize(in_TBN * (normal * 2.0 - 1.0));
    }

    if ((instanceFlags & INSTANCE_FLAG_NORMAL_TEXTURE_INVERT_Y) != 0)
    {
        normal = vec3(normal.r, 1 - normal.g, normal.b);
        n = normalize(in_TBN * (normal * 2.0 - 1.0));
    }

    if ((instanceFlags & INSTANCE_FLAG_NORMAL_TBN) != 0)
    {
        n = in_TBN[2];
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
