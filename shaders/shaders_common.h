// mandatory shader include for all shaders in DRE

layout(set = 0, binding = 0) uniform sampler    g_GlobalSamplers[];
layout(set = 0, binding = 1) buffer             PersistentStorage
{
    float data;
} g_PersistentStorage;

#define GetPersistentData(i) g_PersistentStorage.data[i]

layout(set = 1, binding = 0) uniform texture2D  g_GlobalTextures[];
#include "global_uniform.h" // layout(set = 2, binding = 0)

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