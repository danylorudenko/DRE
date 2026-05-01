#pragma once

#include <foundation\Common.hpp>
#include <common\global_illumination\ddgi_common.h>
#include <glm\fwd.hpp>

namespace GFX::DDGI
{
    glm::uvec3  GetProbeCount3D();
    DRE::U32    GetProbeTotalCount();
    DRE::U32    GetProbeDataBufferSize();

    DDGIConstantBuffer GetConstantBuffer();
}
