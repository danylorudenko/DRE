#include "common/shaders_common.slang"
#include "common/utils/poisson.slang"
#include "common/lighting/lighting_model.slang"
#include "common/vertex/dre_vertex_layout.slang"

struct PassUniform
{
    float4x4 light_ViewProjM;
    float4x4 light_InvViewProjM;
};

struct InstanceUniform
{
    float4x4 model_mat;
    float4x4 prev_model_mat;
    uint4    textureID;
};

[[vk::binding(0, 3)]] ConstantBuffer<PassUniform> passUniform;
[[vk::binding(1, 3)]] Texture2D<float4> envMap;

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

struct PSInput
{
    float4                      ndc_pos   : SV_Position;
    [[vk::location(0)]] float3  ray_start : TEXCOORD0;
    [[vk::location(1)]] float3  ray_end   : TEXCOORD1;
};

[shader("vertex")]
PSInput mainVS(DREVertex input)
{
    float REF_INDEX = GetGenericScalar();

    float2 wave_dir = normalize(float2(0.0, 1.0));

    float3 offset_inpos = input.pos;
    offset_inpos.xz += (wave_dir * 0.01);

    float t = GetTimeS();

    float scale = 3.0;
    float wavelength = 40.0;
    float speed = 0.5;

    int complexity = 2;

    float3 wave_pos = input.pos + GenerateComplexWave(input.pos, wave_dir, t, scale, wavelength, speed, complexity);
    float3 offset_wave_pos = offset_inpos + GenerateComplexWave(offset_inpos, wave_dir, t, scale, wavelength, speed, complexity);

    float3 tan = normalize(wave_pos - offset_wave_pos);
    float3 btan = -(float3(-wave_dir.y, 0.0, wave_dir.x));
    float3 norm = cross(btan, tan);

    float3 wpos = mul(instanceUniform.model_mat, float4(wave_pos, 1.0)).xyz;
    float4 ndc_pos = mul(passUniform.light_ViewProjM, float4(wpos, 1.0));

    PSInput output;
    output.ndc_pos = ndc_pos;

    float2 uv = ndc_pos.xy * 0.5 + 0.5;
    float4 env_map_sample = { 0 };
    //float4 env_map_sample = envMap.Sample(GetSamplerLinear(), uv);

    float3 world_norm = mul(instanceUniform.model_mat, float4(norm, 0.0)).xyz;
    float3 refracted_light = refract(GetSunLightDir(), world_norm, REF_INDEX);
    float2 uv_refract_dir = (mul(passUniform.light_ViewProjM, float4(refracted_light, 0.0)).xy) / GetViewportSize();
    float3 ray_pos = env_map_sample.xyz;
    float2 sample_uv = uv;
    for (int i = 0; i < 3; i++)
    {
        if(ray_pos.y < env_map_sample.y || (length(env_map_sample.xyz) < 0.001))
        {
            break;
        }
        else
        {
            ray_pos = env_map_sample.xyz;
            sample_uv += uv_refract_dir;
            //env_map_sample = envMap.Sample(GetSamplerLinear(), sample_uv);
            env_map_sample = { 0 };
        }
    }

    output.ndc_pos = mul(passUniform.light_ViewProjM, float4(env_map_sample.xyz, 1.0));
    output.ray_start = wpos;
    output.ray_end = ray_pos;

    return output;
}

[shader("pixel")]
float mainPS(PSInput input) : SV_Target0
{
    float old_area = length(ddx(input.ray_start) * ddy(input.ray_start));
    float new_area = length(ddx(input.ray_end) * ddy(input.ray_end));

    float ratio = old_area / new_area;

    return ratio * 0.3;
}

