#include "common/shaders_common.h"
#include "common/global_uniform.h"
#include "gizmo_3D.h"
#include "common/vertex/dre_vertex_layout.h"

[shader("vertex")]
PSInput main(VSInput input)
{
    PSInput out;

    out.wpos = mul(cb.m_Model, float4(input.pos, 1.0)).xyz;
    out.normal = input.norm;
    out.color = input.tan;

    float4 ndc_pos = mul(GetCameraViewProjM(), float4(out.wpos, 1.0));
    out.ndc_pos = ndc_pos;

    return out;
}