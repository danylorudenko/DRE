#ifndef __DRE_VERTEX_LAYOUT_H__
#define __DRE_VERTEX_LAYOUT_H__

struct VSInput
{
    [[vk::location(0)]] float3 in_pos  : POSITION;
    [[vk::location(1)]] float3 in_norm : NORMAL;
    [[vk::location(2)]] float3 in_tan  : TANGENT;
    [[vk::location(3)]] float3 in_btan : BINORMAL;
    [[vk::location(4)]] float2 in_uv   : TEXCOORD0;
};

#endif // __DRE_VERTEX_LAYOUT_H__