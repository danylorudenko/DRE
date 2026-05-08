#include <gfx\renderer\DDGI.hpp>

#include <gfx\GraphicsManager.hpp>

#include <common\global_illumination\ddgi_common.slang>

namespace GFX::DDGI
{

glm::uvec3 GetProbeCount3D()
{
    auto const& settings = g_GraphicsManager->GetGraphicsSettings();
    return glm::uvec3(settings.m_DDGIProbeCountX, settings.m_DDGIProbeCountY, settings.m_DDGIProbeCountZ);
}

DRE::U32 GetProbeTotalCount()
{
    glm::uvec3 const ddgiProbeDimentions = GetProbeCount3D();
    return ddgiProbeDimentions.x * ddgiProbeDimentions.y * ddgiProbeDimentions.z;
}

DRE::U32 GetProbeDataBufferSize()
{
    return sizeof(DDGIProbeData) * GetProbeTotalCount();
}

DDGIConstantBuffer GetConstantBuffer()
{
    auto const& settings = g_GraphicsManager->GetGraphicsSettings();
    glm::uvec3 const ddgiProbeDimentions = GetProbeCount3D();
    glm::vec3 const ddgiProbeWorldDistance = glm::vec3(1.0f); // TODO: make this a setting

    DDGIConstantBuffer cb{};
    cb.probesDimentions = ddgiProbeDimentions;
    cb.probesWorldDistance = ddgiProbeWorldDistance;
    return cb;
}

}