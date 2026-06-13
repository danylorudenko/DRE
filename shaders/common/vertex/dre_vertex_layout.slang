#ifndef __DRE_VERTEX_LAYOUT_H__
#define __DRE_VERTEX_LAYOUT_H__

struct DREVertex
{
    [[vk::location(0)]] float3 pos  : POSITION;
    [[vk::location(1)]] float3 norm : NORMAL;
    [[vk::location(2)]] float3 tan  : TANGENT;
    [[vk::location(3)]] float3 btan : BINORMAL;
    [[vk::location(4)]] float2 uv   : TEXCOORD0;
};

#endif // __DRE_VERTEX_LAYOUT_H__