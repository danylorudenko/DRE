#include "common/shaders_common.h"
#include "common/vertex/dre_vertex_layout.h"

struct PSInput
{
    float4                          ndc_pos      : SV_Position;
    [[vk::location(0)]] float3      wpos         : POSITION;
    [[vk::location(1)]] float4      prev_wpos    : TEXCOORD0;
    [[vk::location(2)]] float2      uv           : TEXCOORD1;
    [[vk::location(3)]] float3x3    TBN          : TEXCOORD2;
};

struct PassUniform
{
    float4x4 shadow_VP;
    float4   shadow_size;
    float4   isFFT_XZ_windX;
};
[[vk::binding(0, 3)]] Texture2D<float>      shadowMap;
[[vk::binding(1, 3)]] Texture2D<float4>     forwardColorMap;
[[vk::binding(2, 3)]] Texture2D<float>      depthMap;
[[vk::binding(3, 3)]] Texture2D<float>      heightMap;
[[vk::binding(4, 3)]] ConstantBuffer<PassUniform> passUniform;

float2 GetWaterVertexDims() { return passUniform.isFFT_XZ_windX.yz; }
bool IsFFT() { return passUniform.isFFT_XZ_windX.x > 0.5; }
float WindXDir() { return passUniform.isFFT_XZ_windX.w; }

struct InstanceUniform
{
    float4x4 model_mat;
    float4x4 prev_model_mat;
    uint4    textureID;
};
[[vk::binding(0, 4)]] ConstantBuffer<InstanceUniform> instanceUniform;

float3 GenerateWave(float3 local_pos, float2 dir, float t, float scale, float wavelength, float speed)
{
    float3 offsets = float3(0.0, 0.0, 0.0);

    float phase = dot(local_pos.xz, dir);
    phase *= (2 * PI) / wavelength;

    offsets.x = (scale * sin(t * speed + phase)) * dir.x;
    offsets.y = scale * cos(t * speed + phase);
    offsets.z = (scale * sin(t * speed + phase)) * dir.y;

    return offsets;
}

float3 GenerateComplexWave(float3 local_pos, float2 dir, float t, float scale, float wavelength, float speed, int count)
{
    float3 offsets = float3(0.0, 0.0, 0.0);

    offsets += GenerateWave(local_pos, dir, t, scale, wavelength, speed);

    offsets += GenerateWave(local_pos, normalize(dir + float2(0.2, 0.0)), t, scale / 10.0, wavelength / 5.0, speed / 2.0);
    offsets += GenerateWave(local_pos, normalize(dir + float2(0.1, 0.0)), t, scale / 8.0, wavelength / 4.0, speed / 2.5);
    offsets += GenerateWave(local_pos, float2(-dir.y, dir.x), t, scale / 3.0, wavelength, speed * 2.0);

    return offsets;
}

float2 CalculateWaterHeightUV(float3 local_pos)
{
    return (local_pos.xz + (GetWaterVertexDims() / 2.0)) / GetWaterVertexDims();
}

[shader("vertex")]
PSInput main(VSInput input)
{
    PSInput output;

    float2 wave_dir = normalize(float2(WindXDir(), 1.0));

    float3 offset_inpos = input.pos;
    offset_inpos.xz += (wave_dir * 0.01);

    float t = GetTimeS();

    float scale = 3.0;
    float wavelength = 40.0;
    float speed = 0.5;

    int complexity = 2;

    float3 wave_pos;
    float3 offset_wave_pos;

    float2 custom_uv = CalculateWaterHeightUV(input.pos);

    if(IsFFT())
    {
        float2 offset_uv = CalculateWaterHeightUV(offset_inpos);

        float height = SampleTexture(heightMap, GetSamplerLinear(), custom_uv).r;
        float offset_height = SampleTexture(heightMap, GetSamplerLinear(), offset_uv).r;

        wave_pos = input.pos;
        wave_pos.y += height;

        offset_wave_pos = offset_inpos;
        offset_wave_pos.y += offset_height;
    }
    else
    {
        wave_pos = input.pos + GenerateComplexWave(input.pos, wave_dir, t, scale, wavelength, speed, complexity);
        offset_wave_pos = offset_inpos + GenerateComplexWave(offset_inpos, wave_dir, t, scale, wavelength, speed, complexity);
    }

    float3 tan = normalize(wave_pos - offset_wave_pos);
    float3 btan = -(float3(-wave_dir.y, 0.0, wave_dir.x));
    float3 norm = cross(btan, tan);

    output.wpos = mul(instanceUniform.model_mat, float4(wave_pos, 1.0)).xyz;

    float4 ndc_pos = mul(GetCameraViewProjM(), float4(output.wpos, 1.0));
    ndc_pos.xy += (GetJitter() * ndc_pos.w);

    output.ndc_pos = ndc_pos;
    output.prev_wpos = mul(instanceUniform.prev_model_mat, float4(wave_pos, 1.0));

    output.uv = custom_uv;
    output.TBN = float3x3(tan, btan, norm);

    return output;
}
