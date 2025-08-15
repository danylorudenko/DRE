#ifndef _RAY_TRACING_INLINE_H_
#define _RAY_TRACING_INLINE_H_

struct S_RAY
{
    float3 origin;
    float3 dir;
};

struct S_RAY_TRACING_RESULT
{
    float3 wpos;
    float3 normal;
    float3 diffuse;
    float  roughness;
    float  metalness;
};

bool TraceRayOpaue(RaytracingAccelerationStructure TLAS, S_RAY initialRay, out S_RAY_TRACING_RESULT result)
{
    RayQuery<RAY_FLAG_NONE> rayQuery;
    rayQuery.TraceRayInline(
        TLAS,
        RAY_FLAG_NONE,
        0xFFFFFFFF,
        initialRay.origin,
        0.1f,
        initialRay.dir,
        10000.0f);

    while(rayQuery.Proceed()) {}

    if (rayQuery.CommittedStatus() == RAY_QUERY_COMMITTED_TRIANGLE_HIT)
    {
        float tHit = rayQuery.CommittedRayT();

        result.wpos = float3(0.0f, 0.0f, 0.0f);
        result.diffuse = float3(0.0f, 0.0f, 0.0f);
        result.normal = float3(0.0f, 0.0f, 0.0f);
        result.roughness = 0.0f;
        result.metalness = 0.0f;

        return true;
    }
    else
    {
        result.wpos = float3(0.0f, 0.0f, 0.0f);
        result.diffuse = float3(0.0f, 0.0f, 0.0f);
        result.normal = float3(0.0f, 0.0f, 0.0f);
        result.roughness = 0.0f;
        result.metalness = 0.0f;

        return false;
    }
}

#endif // _RAY_TRACING_INLINE_H_