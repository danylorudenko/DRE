#pragma once

#include <foundation\Common.hpp>
#include <common\global_illumination\ddgi_common.slang>
#include <glm\fwd.hpp>

namespace GFX::DDGI
{
    glm::uvec3  GetProbeCount3D();
    DRE::U32    GetProbeTotalCount();
    DRE::U32    GetProbeDataBufferSize();

    DDGIConstantBuffer GetConstantBuffer(DRE::U32 probeSphereVertexCount, DRE::U32 probeSphereIndexCount);

    inline char const* GetProbeDebugSphereGeometryName() { return "dre_sphere"; }
}
