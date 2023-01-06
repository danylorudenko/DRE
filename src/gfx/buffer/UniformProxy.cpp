#include <gfx\buffer\UniformProxy.hpp>

#include <foundation\Common.hpp>

#include <vk_wrapper\pipeline\Dependency.hpp>
#include <gfx\GraphicsManager.hpp>

namespace GFX
{

UniformProxy::UniformProxy(VKW::Context* context, UniformArena::Allocation const& allocation)
    : m_Context{ context }
    , m_Allocation{ allocation }
    , m_WritePtr{ m_Allocation.m_MappedRange }
{
}

UniformProxy::~UniformProxy()
{
    m_Allocation.FlushCaches();

    //std::uint32_t const queueFamily = m_Context->GetParentQueue()->GetQueueFamily(); // TODO
    //
    //VKW::Dependency dependency;
    //dependency.Add(m_Allocation.m_Buffer,
    //    m_Allocation.m_OffsetInBuffer, m_Allocation.m_Size,
    //    VKW::RESOURCE_ACCESS_HOST_WRITE,    VKW::STAGE_HOST,                        queueFamily,
    //    VKW::RESOURCE_ACCESS_SHADER_READ,   VKW::STAGE_VERTEX | VKW::STAGE_COMPUTE, queueFamily);
    //
    //m_Context->CmdPipelineBarrier(dependency);

    m_Context->CmdResourceDependency(m_Allocation.m_Buffer, m_Allocation.m_OffsetInBuffer, m_Allocation.m_Size,
        VKW::RESOURCE_ACCESS_HOST_WRITE,    VKW::STAGE_HOST,
        VKW::RESOURCE_ACCESS_SHADER_READ,   VKW::STAGE_VERTEX | VKW::STAGE_COMPUTE);
}



}

