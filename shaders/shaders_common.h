// mandatory shader include for all shaders in DRE

layout(set = 0, binding = 0) uniform sampler    g_GlobalSamplers[];
layout(set = 0, binding = 1) buffer             PersistentStorage
{
    float data;
} g_PersistentStorage;

#define GetPersistentData(i) g_PersistentStorage.data[i]

layout(set = 1, binding = 0) uniform texture2D  g_GlobalTextures[];
#include "global_uniform.h" // layout(set = 2, binding = 0)



