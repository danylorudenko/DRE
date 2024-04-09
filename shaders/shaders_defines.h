// description

#ifndef __SHADER_DEFINES_H__
#define __SHADER_DEFINES_H__

#ifdef __cplusplus
using vec4 = glm::vec4;
using ivec4 = glm::ivec4;
using uvec4 = glm::uvec4;
#endif // __cplusplus

#define PI 3.14159

// Global textures
#define GetGlobalTexture(id) g_GlobalTextures[nonuniformEXT(id)]

// Default samplers
#define GetSamplerNearest() g_GlobalSamplers[0]
#define GetSamplerLinear() g_GlobalSamplers[1]
#define GetSamplerLinearClamp() g_GlobalSamplers[2]
#define GetSamplerAnisotropic() g_GlobalSamplers[3]

#define SampleTexture(textureObj, samplerObj, uv_coords) texture(sampler2D(textureObj, samplerObj), uv_coords)
#define TexelFetchLvl(textureObj, pos, lvl) texelFetch(sampler2D(textureObj, GetSamplerNearest(), pos, lvl)
#define TexelFetch(textureObj, pos) texelFetch(sampler2D(textureObj, GetSamplerNearest()), pos, 0)

#ifdef __cplusplus
#define DeclareStorageBuffer(Type) struct Type
#else
#define DeclareStorageBuffer(Type) layout(buffer_reference, std430, buffer_reference_align = 16) buffer Type
#endif

#endif // __SHADER_DEFINES_H__