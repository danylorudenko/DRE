#ifndef __DRE_VERTEX_LAYOUT_H__
#define __DRE_VERTEX_LAYOUT_H__

struct VSInput
{
    float3 in_pos  : POSITION;
    float3 in_norm : NORMAL;
    float3 in_tan  : TANGENT;
    float3 in_btan : BINORMAL;
    float2 in_uv   : TEXCOORD0;
};

#endif // __DRE_VERTEX_LAYOUT_H__