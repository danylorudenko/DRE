#pragma once

#include <foundation\Common.hpp>
#include <common\global_illumination\ddgi_common.h>

namespace GFX::DDGI
{
    DRE::U32 GetProbeDataBufferSize();

    DDGIConstantBuffer GetConstantBuffer();
}
