#include "common/shaders_common.slang"

#include "common/global_illumination/ddgi_common.slang"
#include "common/lighting/lighting.slang"
#include "common/utils/octahedron.slang"
#include "common/vertex/dre_vertex_layout.slang"

[[vk::binding(0, 3)]] ConstantBuffer<DDGIConstantBuffer>    ddgiCB;
[[vk::binding(1, 3)]] RWStructuredBuffer<DDGIProbeData>     ddgiProbeData;
[[vk::binding(2, 3)]] RWTexture2D                           ddgiGBufferA;
[[vk::binding(3, 3)]] RWTexture2D                           ddgiGBufferB;
[[vk::binding(4, 3)]] RWTexture2D                           ddgiAtlasVisibility;


uint2 CalculateIrradianceTargetPixel(uint probeIndex, float2 probeLocalUV)
{
    uint4 probeGBufferRect = GetDDGIProbeRect(probeIndex, ddgiCB.probeGBufferResolution, ddgiCB.probeGridDimentions);
    uint2 probeInnerRes = ddgiCB.probeGBufferResolution.xx - 2;
    uint2 innerStart = probeGBufferRect.xy + 1;
    uint2 targetPixel = innerStart + uint2(probeLocalUV * probeInnerRes); // maybe needs a clamp to innerRes

    return targetPixel;
}

uint2 CalculateVisibilityTargetPixel(uint probeIndex, float2 probeLocalUV)
{
    uint4 probeVisibilityRect = GetDDGIProbeRect(probeIndex, ddgiCB.probeVisibilityResolution, ddgiCB.probeGridDimentions);
    uint2 probeVisibilityInnerRes = ddgiCB.probeVisibilityResolution.xx - 2;
    uint2 visibilityInnerStart = probeVisibilityRect.xy + 1;
    uint2 visibilityTargetPixel = visibilityInnerStart + uint2(probeLocalUV * probeVisibilityInnerRes); // maybe needs a clamp to innerRes
    
    return visibilityTargetPixel;
}

void ApplyRadianceTrace(uint2 targetPixel, S_INSTANCE* hitInstance, float2 hitGeometryUV, float3x3 TBN)
{
    S_MATERIAL *material = GetMaterial(hitInstance);

    ComputeMaterialTextureSampler sampler = { 0.0f };
    S_MATERIAL_PROPERTIES MaterialProperties = ReadMaterialProperties(material, hitGeometryUV, TBN, sampler);

    ddgiGBufferA[targetPixel] = float4(MaterialProperties.diffuse, MaterialProperties.roughness);
    ddgiGBufferB[targetPixel] = float4(MaterialProperties.normal * 0.5 + 0.5, MaterialProperties.metalness);
}

void ApplyVisibilityTrace(uint2 targetPixel, float rayT)
{
    ddgiAtlasVisibility[targetPixel] = rayT;
}

void TraceProbeRay()
{
    
}


[require(spvRayQueryKHR)]
[shader("compute")]
[numthreads(4, 4, 4)]
void mainCS(uint3 DTid : SV_DispatchThreadID)
{
    uint probeIndex = GetProbeIndex(ddgiCB.probeGridDimentions, DTid);

    // 1. generate noise seed
    uint probeSeed = probeIndex + GetFrameNumber();

    // 2. based on noise generate random direction
    float alpha = PcgHash(probeSeed) * (1.0/float(0xFFFFFFFFu)) * PI;
    float theta = PcgHash(probeSeed) * (1.0/float(0xFFFFFFFFu)) * PI * 2.0;

    DDGIProbeData probeData = ddgiProbeData[probeIndex];
    float3 rayOrigin = probeData.position.xyz;
    float3 rayDir = PolarToCartesian(alpha, theta);

    float2 probeLocalUV = OctahedronEncode(rayDir);

    // 3. trace ray from the probe location towards the chosen direction
    RayDesc ray;
    ray.Origin = rayOrigin;
    ray.Direction = rayDir;
    ray.TMin = 0.01;
    ray.TMax = 1000.0;

    uint2 irradianceTargetPixel = CalculateIrradianceTargetPixel(probeIndex, probeLocalUV);
    uint2 visibilityTargetPixel = CalculateVisibilityTargetPixel(probeIndex, probeLocalUV);

    RayQuery<RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> rayQuery;
    rayQuery.TraceRayInline(g_TLAS, RAY_FLAG_NONE, 0xFFFFFFFF, ray);

    rayQuery.Proceed();

    if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        // 3.1 from the primitive index, get triangle indicies
        uint instanceID = rayQuery.CommittedInstanceID();
        S_INSTANCE* instance = GetInstance(instanceID);

        uint primitiveID = rayQuery.CommittedPrimitiveIndex();
        uint* indexOffset = (uint*)GetGlobalGeometry(instance->indexOffset + primitiveID * 3);

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
        if (ddgiCB.debugDrawRays != 0 && all(DTid.xyz == ddgiCB.debugProbeFocus))
        {
            DrawDebugLine(rayOrigin, wPos, float3(1, 1, 0));
            DrawDebugCross(wPos, 1.0f, float3(0, 1, 0));
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

        ApplyRadianceTrace(irradianceTargetPixel, instance, hitUV, TBN);
        ApplyVisibilityTrace(visibilityTargetPixel, rayQuery.CommittedRayT());
    }
    else
    {
        ddgiGBufferA[irradianceTargetPixel] = float4(0, 0, 0, 1);
        ddgiGBufferB[irradianceTargetPixel] = float4(0, 0, 1, 0);

        ddgiAtlasVisibility[visibilityTargetPixel] = ray.TMax;

        if (ddgiCB.debugDrawRays != 0 && all(DTid.xyz == ddgiCB.debugProbeFocus))
        {
            DrawDebugLine(rayOrigin, rayOrigin + rayDir * ray.TMax, float3(1, 0, 0));
        }
    }

}