#include <gfx\renderer\DDGI.hpp>

#include <gfx\GraphicsManager.hpp>

#include <common\global_illumination\ddgi_common.h>

namespace GFX::DDGI
{

DRE::U32 GetProbeDataBufferSize()
{
    auto const& settings = g_GraphicsManager->GetGraphicsSettings();
    glm::uvec3 const ddgiProbeDimentions = glm::uvec3(settings.m_DDGIProbeCountX, settings.m_DDGIProbeCountY, settings.m_DDGIProbeCountZ);
    return sizeof(DDGIProbeData) * ddgiProbeDimentions.x * ddgiProbeDimentions.y * ddgiProbeDimentions.z;
}

DDGIConstantBuffer GetConstantBuffer()
{
    auto const& settings = g_GraphicsManager->GetGraphicsSettings();
    glm::uvec3 const ddgiProbeDimentions = glm::uvec3(settings.m_DDGIProbeCountX, settings.m_DDGIProbeCountY, settings.m_DDGIProbeCountZ);
    glm::vec3 const ddgiProbeWorldDistance = glm::vec3(1.0f); // TODO: make this a setting

    DDGIConstantBuffer cb{};
    cb.probesDimentions = ddgiProbeDimentions;
    cb.probesWorldDistance = ddgiProbeWorldDistance;
    return cb;
}

}