// mandatory shader include for all shaders in DRE
#ifndef _SHADERS_COMMON_H_
#define _SHADERS_COMMON_H_

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference2    : enable

#include "shaders_defines.h"

layout(set = 0, binding = 0) uniform sampler    g_GlobalSamplers[];
layout(set = 1, binding = 0) uniform texture2D  g_GlobalTextures[];
#include "global_uniform.h" // layout(set = 2, binding = 0)


// Global textures
#define GetGlobalTexture(id) g_GlobalTextures[nonuniformEXT(id)]

// Default samplers
#define GetSamplerNearest() g_GlobalSamplers[0]
#define GetSamplerLinear() g_GlobalSamplers[1]
#define GetSamplerLinearClamp() g_GlobalSamplers[2]
#define GetSamplerAnisotropic() g_GlobalSamplers[3]

// Texture sampling
#define SampleTexture(textureObj, samplerObj, uv_coords) texture(sampler2D(textureObj, samplerObj), uv_coords)
#define TexelFetchLvl(textureObj, pos, lvl) texelFetch(sampler2D(textureObj, GetSamplerNearest(), pos, lvl)
#define TexelFetch(textureObj, pos) texelFetch(sampler2D(textureObj, GetSamplerNearest()), pos, 0)

float sRGB2Linear(float x)
{
    return pow(x, 1.0 / 2.2);
}

vec3 sRGB2Linear(vec3 x)
{
    return pow(x, vec3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));
}

float Linear2sRGB(float x)
{
    return pow(x, 2.2);
}

vec3 Linear2sRGB(vec3 x)
{
    return pow(x, vec3(2.2, 2.2, 2.2));
}

// Global Texture sampling
vec4 SampleGlobalTextureLinear(uint id, vec2 uv)
{
    return SampleTexture(GetGlobalTexture(id), GetSamplerLinear(), uv);
}

vec4 SampleGlobalTextureAnisotropic(uint id, vec2 uv)
{
    return SampleTexture(GetGlobalTexture(id), GetSamplerAnisotropic(), uv);
}

#endif // _SHADERS_COMMON_H_