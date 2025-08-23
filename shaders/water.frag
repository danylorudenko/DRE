#include "common/shaders_common.h"
#include "common/utils/poisson.h"
#include "common/lighting/lighting_model.h"

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

#define NormalTextureID instanceUniform.textureID[0]

float3 FresnelShlickWater(float NdotH, float3 F0)
{
    return F0 + (float3(1.0, 1.0, 1.0) - F0) * pow(1.0 - NdotH, 5.0);
}

float3 WaterSpecular(float NdotH, float NdotV, float NdotL, float3 F0, float roughness)
{
    float NDF = GGX_NDF(NdotH, roughness);
    float G = SmithGGX(NdotV, NdotL, roughness);
    float3 F = FresnelShlickWater(NdotL, F0);

    float3 numerator = NDF * G * F;
    float denum = 4.0 * NdotV * NdotL + 0.001;

    float3 specular = numerator / denum;

    return specular;
}

float map(float value, float min1, float max1, float min2, float max2)
{
    return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

float RaymarchWaterDepth(float3 wpos, float currentDepth)
{
    const float max_depth = 1.0;
    const float step = 0.1;
    const int step_count = int(max_depth / step);
    const float3 start = float3(wpos.x, wpos.y - max_depth, wpos.z);

    float depth_estimate = max_depth;

    float prev_depth_sample = 0.0;
    float3 prev_test_wpos = float3(0.0, 0.0, 0.0);
    for(int i = step_count - 1; i >= 0; i--)
    {
        float3 test_wpos = wpos - float3(0.0, step * i, 0.0);
        float4 test_viewpos = mul(GetCameraViewProjM(), float4(test_wpos, 1.0));
        float3 test_ndc = test_viewpos.xyz / test_viewpos.w;
        float2 test_uv = test_ndc.xy * 0.5 + 0.5;
        float depth_sample = SampleTexture(depthMap, GetSamplerLinearClamp(), test_uv).r;
        if(depth_sample < currentDepth)
        {
            float delta = (currentDepth - depth_sample);
            float mixer = (delta / step) / (currentDepth);
            float last_sample_depth = (wpos.y - test_wpos.y);
            float prev_sample_depth = (wpos.y - prev_test_wpos.y);
            return lerp(last_sample_depth, prev_sample_depth, mixer);
        }

        prev_depth_sample = depth_sample;
        prev_test_wpos = test_wpos;
    }

    return max_depth;
}

float LinearizeDepth(float d, float zNear, float zFar)
{
    float x = (zFar - zNear) / zNear;
    return 1.0 / (x * d + 1.0);
}

float SimpleWaterDepth(float currentDepth, float comparedDepth)
{
    return (comparedDepth - currentDepth);
}

float3 CausticContribution(float3 wpos, float3 l, float3 v, float3 n)
{
    float3 result = float3(0.0, 0.0, 0.0);

    float3 vFlat = normalize(float3(v.x, 0.0, v.z));
    float3 offset = vFlat * dot(v, vFlat);

    result = dot(n, float3(0.0, 1.0, 0.0)).xxx;
    result = offset;

    return result;
}

struct PSOutput
{
    float4 finalColor : SV_Target0;
    float2 velocity   : SV_Target1;
};

[shader("pixel")]
PSOutput main(PSInput input)
{
    const float3 diffuse = float3(0.0, 57.0, 74.0) / 255.0;
    const float3 F0 = float3(39.0, 39.0, 39.0) / 255.0;
    float2 windDir = normalize(float2(WindXDir(), 1.0));

    float2 pixel_pos_uv = input.ndc_pos.xy / GetViewportSize();

    float3 normalMap0 = sRGB2Linear(SampleGlobalTextureLinear(NormalTextureID, input.wpos.xz / 10.0 + windDir * (GetTimeS() / 48.0)).rgb);
    float3 normalMap1 = sRGB2Linear(SampleGlobalTextureLinear(NormalTextureID, input.wpos.zx / 4.0 + windDir * (GetTimeS() / 44.0)).rgb);

    float3 normalMap = normalize(normalMap0 + normalMap1);

    normalMap = (normalMap * 2.0 - 1.0);
    float3 n = normalize(mul(input.TBN, normalMap));

    float3 l = -GetSunLightDir();
    float3 v = normalize(GetCameraPos() - input.wpos);
    float3 h = normalize(l + v);

    float NdotL = max(0.0, dot(n, l));
    float NdotV = max(0.0, dot(n, v));
    float NdotH = max(0.0, dot(n, h));

    float3 specular = WaterSpecular(NdotH, NdotV, NdotL, F0, 0.05);

    float sampledDepthLinear = LinearizeDepth(SampleTexture(depthMap, GetSamplerLinearClamp(), pixel_pos_uv).r, 0.1, 100.0);
    float currentDepthLinear = LinearizeDepth(input.ndc_pos.z, 0.1, 100.0);
    float waterEyeDepth = SimpleWaterDepth(currentDepthLinear, sampledDepthLinear);

    float2 refracted_sample_pos = pixel_pos_uv;
    refracted_sample_pos += normalMap.xy * 0.1 * clamp(waterEyeDepth * 50.0, 0.0, 1.5);

    float depthSampleRefractedLinear = LinearizeDepth(SampleTexture(depthMap, GetSamplerLinear(), refracted_sample_pos).r, 0.1, 100.0);
    if((currentDepthLinear - depthSampleRefractedLinear) > 0.0)
    {
        refracted_sample_pos = pixel_pos_uv;
        depthSampleRefractedLinear = sampledDepthLinear;
    }

    float3 worldSampleRefracted = SampleTexture(forwardColorMap, GetSamplerLinear(), refracted_sample_pos).rgb;
    float3 worldSample = worldSampleRefracted;

    float refractedWaterDepth = SimpleWaterDepth(currentDepthLinear, depthSampleRefractedLinear);
    float3 water_diffuse = lerp(worldSample, diffuse, clamp(refractedWaterDepth * 30.0 + 0.2, 0.0, 1.0));

    float3 res = specular + water_diffuse;

    PSOutput output;
    output.finalColor = float4(res, 1.0);

    float4 prev_ndc = mul(GetPrevCameraViewProjM(), input.prev_wpos);
    prev_ndc /= prev_ndc.w;

    float2 pixel_pos_ndc = pixel_pos_uv * 2.0 - 1.0;
    float2 vel = (pixel_pos_ndc - prev_ndc.xy);
    float2 vel_uv = vel * 0.5;

    output.velocity = vel_uv;
    return output;
}

