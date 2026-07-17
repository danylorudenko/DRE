#include "common/shaders_common.slang"

#include "common/global_illumination/ddgi_common.slang"
#include "common/lighting/lighting.slang"
#include "common/utils/octahedron.slang"
#include "common/vertex/dre_vertex_layout.slang"

[[vk::binding(0, 3)]] ConstantBuffer<DDGIConstantBuffer>    ddgiCB;
[[vk::binding(1, 3)]] RWStructuredBuffer<DDGIProbeData>     ddgiProbeData;
[[vk::binding(2, 3)]] RWStructuredBuffer<DDGIProbeSamples>  ddgiSampleBuffer;
[[vk::binding(3, 3)]] RWTexture2D                           ddgiAtlasVisibility;

void AddTracedSample(uint probeIndex, DDGIProbeData probeData, float3 rayDir, float rayT, float3 hitPosition, S_INSTANCE* hitInstance, float2 hitGeometryUV, float3x3 TBN)
{
    S_MATERIAL *material = GetMaterial(hitInstance);

    ComputeMaterialTextureSampler sampler = { 0.0f };
    S_MATERIAL_PROPERTIES MaterialProperties = ReadMaterialProperties(material, hitGeometryUV, TBN, sampler);

    S_SURFACE surface = {};

    surface.diffuseSpectrum = MaterialProperties.diffuse;
    surface.roughness = MaterialProperties.roughness;
    surface.normal = MaterialProperties.normal;
    surface.metalness = MaterialProperties.metalness;
    surface.ao = 1;
    surface.wpos = hitPosition;
    surface.prevWpos = hitPosition;

    S_LIGHTING_RESULT sampleDirectRadiance = CalculateDirectLighting(surface, probeData.position.xyz, ShaderStage::Compute);
    S_LIGHTING_RESULT sampleIndirect = CalculateIndirectLighting(surface, GetDDGI().irradianceHistoryTextureID);

    DDGISample sample;
    sample.radiance = sampleDirectRadiance.finalRadiance + sampleIndirect.finalRadiance;
    //sample.radiance = surface.diffuseSpectrum;
    //sample.radiance = float3(1,1,1);
    sample.distance = rayT;
    sample.octDirection = OctahedronEncode(rayDir);

    uint3 probeIndex3D = GetProbe3DIndex(ddgiCB.probeGridDimentions, probeIndex);
    if (ddgiCB.debugDrawRays != 0 && all(probeIndex3D == ddgiCB.debugProbeFocus))
    {
        DrawDebugCrossAligned(hitPosition, 0.5f, sample.radiance);
    }

    uint sampleIndex;
    /////////////////////////////
    // these line trigger DEVICE_LOST
    InterlockedAdd(ddgiSampleBuffer[probeIndex].sampleCount, 1, sampleIndex);
    ddgiSampleBuffer[probeIndex].samples[sampleIndex] = sample;
    /////////////////////////////
}

void AddSkySample(uint probeIndex, float3 rayDir, float rayT)
{
    DDGISample sample;
    sample.radiance = float3(1.0, 1.0, 1.0);
    sample.distance = rayT;
    sample.octDirection = OctahedronEncode(rayDir);

    uint sampleIndex;
    InterlockedAdd(ddgiSampleBuffer[probeIndex].sampleCount, 1, sampleIndex);
    ddgiSampleBuffer[probeIndex].samples[sampleIndex] = sample;
}

void ApplyVisibilityTrace(uint2 targetPixel, float rayT)
{
    ddgiAtlasVisibility[targetPixel] = rayT;
}

void TraceProbeRay(DDGIProbeData probeData, uint probeIndex, float3 rayOrigin, float3 rayDir)
{
    float2 probeLocalUV = OctahedronEncode(rayDir);
    uint3 probeIndex3D = GetProbe3DIndex(ddgiCB.probeGridDimentions, probeIndex);

    // 3. trace ray from the probe location towards the chosen direction
    RayDesc ray;
    ray.Origin = rayOrigin;
    ray.Direction = rayDir;
    ray.TMin = 0.01;
    ray.TMax = 1000.0;

    uint2 visibilityTargetPixel = CalculateDDGIAtlasTargetPixel(probeIndex, probeLocalUV, ddgiCB.probeVisibilityResolution, ddgiCB.probeGridDimentions);

    RayQuery<RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> rayQuery;
    rayQuery.TraceRayInline(g_TLAS, RAY_FLAG_NONE, 0xFFFFFFFF, ray);

    rayQuery.Proceed();

    if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        if (rayQuery.CommittedTriangleFrontFace())
        {
            // 3.1 from the primitive index, get triangle indicies
            uint instanceID = rayQuery.CommittedInstanceID();
            S_INSTANCE* instance = GetInstance(instanceID);
            
            uint primitiveID = rayQuery.CommittedPrimitiveIndex();
            uint* indexOffset = (uint*)GetGlobalGeometry(instance->indexOffset + sizeof(uint) * primitiveID * 3);
            
            // 3.2 from triangle indicies get vertex attributes
            DREVertex* vertices = (DREVertex*)GetGlobalGeometry(instance->vertexOffset);
            DREVertex v0 = vertices[indexOffset[0]];
            DREVertex v1 = vertices[indexOffset[1]];
            DREVertex v2 = vertices[indexOffset[2]];
            
            // 3.3 calculate 3rd barycentric weight
            float3 barycentric = float3(0.0, rayQuery.CommittedTriangleBarycentrics());
            barycentric.x = 1.0 - (barycentric.y + barycentric.z);
            
            // 3.4 interpolate vertex attributes with barycentrics
            float2 hitUV = v0.uv * barycentric.x + v1.uv * barycentric.y + v2.uv * barycentric.z;
            S_MATERIAL *material = GetMaterial(instance);
            
            float3 wPos = ray.Origin + ray.Direction * rayQuery.CommittedRayT();
            
            if (ddgiCB.debugDrawRays != 0 && all(probeIndex3D == ddgiCB.debugProbeFocus))
                {
                    DrawDebugLine(rayOrigin, wPos, float3(1, 1, 0));
                    DrawDebugCross(wPos, 0.5f, float3(0, 1, 0));
                }
                
                float3 T = v0.tan * barycentric.x + v1.tan * barycentric.y + v2.tan * barycentric.z;
                float3 B = v0.btan * barycentric.x + v1.btan * barycentric.y + v2.btan * barycentric.z;
                float3 N = v0.norm * barycentric.x + v1.norm * barycentric.y + v2.norm * barycentric.z;
                
                float4x4 model_mat = GetWorldTransform(instance);
                float3x3 TBN = float3x3(
                    normalize(mul(model_mat, float4(T, 0.0)).xyz),
                    normalize(mul(model_mat, float4(B, 0.0)).xyz),
                    normalize(mul(model_mat, float4(N, 0.0)).xyz)
                );
                
                AddTracedSample(probeIndex, probeData, ray.Direction, rayQuery.CommittedRayT(), wPos, instance, hitUV, TBN);
        }
    }
    else
    {
        AddSkySample(probeIndex, ray.Direction, ray.TMax);
        ddgiAtlasVisibility[visibilityTargetPixel] = ray.TMax;

        if (ddgiCB.debugDrawRays != 0 && all(probeIndex3D == ddgiCB.debugProbeFocus))
        {
            DrawDebugLine(rayOrigin, rayOrigin + rayDir * ray.TMax, float3(1, 0, 0));
        }
    }
}


[require(spvRayQueryKHR)]
[shader("compute")]
[numthreads(4, 4, 4)]
void mainCS(uint3 DTid : SV_DispatchThreadID)
{
    if (any(DTid >= ddgiCB.probeGridDimentions))
        return;

    uint probeIndex = GetProbeIndex(ddgiCB.probeGridDimentions, DTid);

    uint probeSeed = probeIndex + GetFrameNumber();

    for (uint i = 0; i < ddgiCB.rayCountPerProbe; i++)
    {
        float alpha = PcgHash(probeSeed) * (1.0/float(0xFFFFFFFFu)) * PI;
        float theta = PcgHash(probeSeed) * (1.0/float(0xFFFFFFFFu)) * PI * 2.0;

        DDGIProbeData probeData = ddgiProbeData[probeIndex];
        float3 rayOrigin = probeData.position.xyz;
        float3 rayDir = PolarToCartesian(alpha, theta);
        //float3 rayDir = float3(0, 0, 1);

        float2 probeLocalUV = OctahedronEncode(rayDir);

        TraceProbeRay(probeData, probeIndex, rayOrigin, rayDir);
    }
}