#ifndef _GIZMO_3D_H_
#define _GIZMO_3D_H_

#include "common/shaders_common.h"

struct GizmoPassBuffer
{
    float4x4 m_Model;
};
DeclareConstantBuffer(GizmoPassBuffer, cb, 3, 0);

#ifndef __cplusplus
//float GetCameraDistance() { return cb.m_CameraDistance.x; }
#endif

#endif // _GIZMO_3D_H_
