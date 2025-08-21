#include "common/shaders_common.h"
#include "gizmo_3D.h"

[shader("pixel")]
float4 main(PSInput input) : SV_Target0
{
    float3 L = normalize(float3(1.0, 1.0, 0.0));
    float3 n = normalize(input.normal);

    float toWhite = dot(n, L) * 0.5 + 0.5;

    float3 white = float3(1.0, 1.0, 1.0);
    return float4(lerp(input.color, white, toWhite), 1.0);
}
