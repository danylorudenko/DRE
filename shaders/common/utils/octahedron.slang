#ifndef _OCTAHEDRON_SLANG_
#define _OCTAHEDRON_SLANG_

float2 OctahedronWrap(float2 p)
{
    return (1.0 - abs(p.yx)) * select(p >= 0.0f, 1.0f, -1.0f);
}

float2 OctahedronEncode(float3 sphereDir)
{
    float3 octahedron = sphereDir / (abs(sphereDir.x) + abs(sphereDir.y) + abs(sphereDir.z));
    float2 xy = octahedron.z >= 0.0f ? octahedron.xy : OctahedronWrap(octahedron.xy);
    float2 uv = xy * 0.5 + 0.5;

    return uv;
}

float3 OctahedronDecode(float2 uv)
{
    float2 xy = uv * 2.0 - 1.0;

    // https://twitter.com/Stubbesaurus/status/937994790553227264
    float3 n = float3(xy.x, xy.y, 1.0 - abs(xy.x) - abs(xy.y));
    float t = saturate(-n.z);
    n.xy += select(n.xy >= 0.0,-t, t);
    return normalize(n);
}

#endif // _OCTAHEDRON_SLANG_
