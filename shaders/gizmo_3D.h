#ifndef _GIZMO_3D_H_
#define _GIZMO_3D_H_

#include "common/shaders_common.h"

struct GizmoPassBuffer
{
    float4x4 m_Model;
};

#ifndef __cplusplus
[[vk::binding(0, 3)]] ConstantBuffer<GizmoPassBuffer> cb;
#endif

#endif // _GIZMO_3D_H_
