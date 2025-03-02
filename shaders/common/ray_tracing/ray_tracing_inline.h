#ifndef _RAY_TRACING_INLINE_H_
#define _RAY_TRACING_INLINE_H_

struct S_RAY
{
    vec3 origin;
    vec3 dir;
};

struct S_RAY_TRACING_RESULT
{
    vec3 wpos;
    vec3 normal;
    vec3 diffuse;
    float roughness;
    float metalness;
};

bool TraceRayOpaue(accelerationStructureEXT TLAS, S_RAY initialRay, out S_RAY_TRACING_RESULT result)
{
    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(
        rayQuery,
        TLAS,
        gl_RayFlagsNoneEXT,
        0xFFFFFFFF,
        initialRay.origin,
        0.1f, // tMin
        initialRay.dir,
        10000); // tMax

    while(rayQueryProceedEXT(rayQuery))
    {
    }

    if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT)
    {
        float tHit = rayQueryGetIntersectionTMinEXT(rayQuery);

        // rayQueryGetIntersectionGeometryIndex(rayQuery); - geometry id
        // rayQueryGetIntersectionPrimitiveIndex(rayQuery); - triangle id

        result.wpos = vec3(0.0f, 0.0f, 0.0f);
        result.diffuse = vec3(0.0f, 0.0f, 0.0f);
        result.normal = vec3(0.0f, 0.0f, 0.0f);
        result.roughness = 0.0f;
        result.metalness = 0.0f;

        return true;
    }
    else
    {
        result.wpos = vec3(0.0f, 0.0f, 0.0f);
        result.diffuse = vec3(0.0f, 0.0f, 0.0f);
        result.normal = vec3(0.0f, 0.0f, 0.0f);
        result.roughness = 0.0f;
        result.metalness = 0.0f;

        return false;
    }
}

#endif // _RAY_TRACING_INLINE_H_