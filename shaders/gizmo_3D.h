#ifndef _GIZMO_3D_H_
#define _GIZMO_3D_H_

#include "common/shaders_common.h"

struct GizmoPassBuffer
{
    float4x4 m_Model;
};

#ifndef __cplusplus
[[vk::binding(0, 3)]] ConstantBuffer<GizmoPassBuffer> cb;

struct PSInput
{
    [[vk::location(0)]] float3  wpos : POSITION;
    [[vk::location(1)]] float3  color : TEXCOORD0;
    [[vk::location(2)]] float3  normal : NORMAL;
    float4                      ndc_pos : SV_Position;
};
#endif

#endif // _GIZMO_3D_H_
