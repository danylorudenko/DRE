// description

#define PI 3.14159

#define GetPersistentData(i) g_PersistentStorage.data[i]

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

vec4 SampleGlobalTextureLinear(uint id, vec2 uv)
{
    return SampleTexture(GetGlobalTexture(id), GetSamplerLinear(), uv);
}

vec4 SampleGlobalTextureAnisotropic(uint id, vec2 uv)
{
    return SampleTexture(GetGlobalTexture(id), GetSamplerAnisotropic(), uv);
}


