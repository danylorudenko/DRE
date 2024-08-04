#ifndef _GIZMO_3D_H_
#define _GIZMO_3D_H_

#include "shaders_common.h"

BEGIN_CONSTANT_BUFFER(GizmoPassBuffer, cb, 3, 0)
{
    mat4 m_Model;
    vec4 m_CameraDistance;
}
END_CONSTANT_BUFFER(GizmoPassBuffer, cb, 3, 0)

#ifndef __cplusplus
float GetCameraDistance() { return cb.m_CameraDistance.x; }
#endif

#endif // _GIZMO_3D_H_