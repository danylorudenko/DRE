// description

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


