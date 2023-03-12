// description

#define PI 3.14159

const vec2 poisson32[32] =
{
    vec2(-0.397889, 0.542226),
    vec2(-0.414755, -0.394183),
    vec2(0.131764, -0.713506),
    vec2(0.551543, 0.554334),
    vec2(0.317522, -0.088899),
    vec2(0.927145, 0.283128),
    vec2(0.141766, 0.672284),
    vec2(-0.626308, 0.079957),
    vec2(-0.379704, -0.823208),
    vec2(-0.165635, 0.116704),
    vec2(0.477730, -0.835368),
    vec2(0.823137, -0.082292),
    vec2(-0.254509, 0.914898),
    vec2(-0.029949, -0.332681),
    vec2(-0.735420, 0.649945),
    vec2(0.269829, 0.337499),
    vec2(0.589355, 0.188804),
    vec2(0.495027, -0.463772),
    vec2(0.430761, 0.880621),
    vec2(-0.740073, -0.226115),
    vec2(-0.843081, 0.319486),
    vec2(-0.118380, 0.503956),
    vec2(-0.103058, -0.967695),
    vec2(-0.989892, 0.031239),
    vec2(-0.650113, -0.657721),
    vec2(-0.395081, -0.071884),
    vec2(-0.409406, 0.272306),
    vec2(0.112218, 0.112523),
    vec2(0.258025, -0.346162),
    vec2(0.105651, 0.945739),
    vec2(-0.164829, -0.660185),
    vec2(0.700367, -0.693439)
};

const vec2 poisson16[16] =
{
    vec2(-0.376812, 0.649265),
    vec2(-0.076855, -0.632508),
    vec2(-0.833781, -0.268513),
    vec2(0.398413, 0.027787),
    vec2(0.360999, 0.766915),
    vec2(0.584715, -0.809986),
    vec2(-0.238882, 0.067867),
    vec2(0.824410, 0.543863),
    vec2(0.883033, -0.143517),
    vec2(-0.581550, -0.809760),
    vec2(-0.682282, 0.223546),
    vec2(0.438031, -0.405749),
    vec2(0.045340, 0.428813),
    vec2(-0.311559, -0.328006),
    vec2(-0.054146, 0.935302),
    vec2(0.723339, 0.196795)
};

layout(set = 0, binding = 0) uniform sampler    g_GlobalSamplers[];
layout(set = 0, binding = 1) buffer             PersistentStorage
{
    float data;
} g_PersistentStorage;

#define GetPersistentData(i) g_PersistentStorage.data[i]

layout(set = 1, binding = 0) uniform texture2D  g_GlobalTextures[];
#include "global_uniform.h" // layout(set = 2, binding = 0)

// Global textures
#define GetGlobalTexture(id) g_GlobalTextures[nonuniformEXT(id)]

// Default samplers
#define GetSamplerNearest() g_GlobalSamplers[0]
#define GetSamplerLinear() g_GlobalSamplers[1]
#define GetSamplerLinearClamp() g_GlobalSamplers[2]
#define GetSamplerAnisotropic() g_GlobalSamplers[3]

#define SampleTexture(textureObj, samplerObj, uv_coords) texture(sampler2D(textureObj, samplerObj), uv_coords)

vec4 SampleGlobalTextureLinear(uint id, vec2 uv)
{
    return SampleTexture(GetGlobalTexture(id), GetSamplerLinear(), uv);
}

vec4 SampleGlobalTextureAnisotropic(uint id, vec2 uv)
{
    return SampleTexture(GetGlobalTexture(id), GetSamplerAnisotropic(), uv);
}


