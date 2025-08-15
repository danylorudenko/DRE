// Resources
[[vk::binding(0, 3)]]
Texture2D<float4> linearRT;

[[vk::binding(1, 3)]]
RWTexture2D<float4> encodedOutput;

// Uniforms
[[vk::binding(2, 3)]]
cbuffer PassUniform
{
    float4 useEncoded_exposure; // .x = useEncoded, .y = exposure
};

float GetUseEncoded() { return useEncoded_exposure.x; }
float GetExposure()   { return useEncoded_exposure.y; }

// ACES filmic tone map
float3 ACESFilm(float3 x)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// Compute kernel
[numthreads(8, 8, 1)]
void mainCS(uint3 DTid : SV_DispatchThreadID)
{
    uint2 coords = DTid.xy;

    float3 linearColor  = linearRT.Load(int3(coords, 0)).rgb;
    float3 encoded = ACESFilm(linearColor * GetExposure());
    float3 result  = lerp(linearColor, encoded, GetUseEncoded());

    encodedOutput[coords] = float4(result, 1.0f);
}