#include "common/shaders_common.slang"

[[vk::binding(1, 3)]] Texture2D<float2>         velocity;
[[vk::binding(2, 3)]] Texture2D<float3>         color_buffer;
[[vk::binding(3, 3)]] Texture2D<float3>         history;
[[vk::binding(4, 3)]] RWTexture2D<float4>       taa_output;
[[vk::binding(5, 3)]] Texture2D<float>          main_depth;


struct PassUniform
{
    float4 alpha_varianceGamma;
};
[[vk::binding(0, 3)]] ConstantBuffer<PassUniform> passUniform;

[numthreads(8, 8, 1)]
void mainCS(int2 DTid : SV_DispatchThreadID)
{
    int2 coords = DTid;
    float2 uv = (float2(coords) + 0.5) / GetViewportSize();
    //float center_depth = main_depth.Sample(GetSamplerNearest(), uv);
    float center_depth = main_depth[coords];

    float2 uv_delta = rcp(GetViewportSize());
    float3 rgb_aabb[2];

    float3 m1 = float3(0.0);
    float3 m2 = float3(0.0);

    float3 velocityUV_depth = float3(uv, center_depth);
    for(int x = -1; x < 2; x++) {
        for(int y = -1; y < 2; y++) {
            float2 neighborhood_uv = uv + uv_delta * float2(x, y);
            //float3 neighborhood = color_buffer.Sample(GetSamplerNearest(), neighborhood_uv);
            float3 neighborhood = color_buffer[coords];
            
            m1 += neighborhood;
            m2 += neighborhood * neighborhood;

            //rgb_aabb[0] = min(rgb_aabb[0], neighborhood);
            //rgb_aabb[1] = max(rgb_aabb[1], neighborhood);

            //float depth = main_depth.Sample(GetSamplerNearest(), neighborhood_uv);
            float depth = main_depth[coords];
            if(depth > velocityUV_depth.z)
                velocityUV_depth = float3(neighborhood_uv, depth);
        }
    }

    const float N = 9;
    float3 mu = m1 / N;
    float3 sigma = sqrt(m2 / N - mu * mu);

    float gamma = passUniform.alpha_varianceGamma.y;
    rgb_aabb[0] = mu - sigma * gamma;
    rgb_aabb[1] = mu + sigma * gamma;

    //float2 vel = velocity.Sample(GetSamplerNearest(), velocityUV_depth.xy);
    float2 vel = velocity[coords];
    float2 uv_prev = uv - vel;    
    //float3 hist_sample = history.Sample(GetSamplerLinear(), uv_prev);
    float3 hist_sample = history[coords];
    //float3 current = color_buffer.Sample(GetSamplerNearest(), uv);
    float3 current = color_buffer[coords];

    float3 rectified_history = clamp(hist_sample, rgb_aabb[0], rgb_aabb[1]);

    float alpha = passUniform.alpha_varianceGamma.x;
    float3 result = lerp(current, rectified_history, alpha);   
    taa_output[coords] = float4(result, 1.0);
}