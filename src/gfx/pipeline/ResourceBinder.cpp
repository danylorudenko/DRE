#include <gfx\pipeline\ResourceBinder.hpp>

namespace GFX
{

ResourceBinder::ResourceBinder(VKW::Device* device, PipelineEntry* pipelineEntry, DRE::U32 setIdAfterGlobalSets, FrameID frameID)
    : m_Device{ device }
    , m_PipelineEntry{ pipelineEntry }
{
    VKW::PipelineLayout* pipelineLayout = pipelineEntry->GetLayout();

#ifdef DRE_DEBUG
    DRE::S16 const privateSetCount = pipelineLayout->GetMemberCount() - device->GetDescriptorManager()->GetGlobalPipelineLayout()->GetMemberCount();
    DRE_ASSERT(privateSetCount >= 0, "There're less descriptor bindings in the pipeline than there should be. Not enough global bindings");
#endif // DRE_DEBUG

    DRE::U32 const targetSet = VKW::DescriptorManager::GLOBAL_SET_COUNT + setIdAfterGlobalSets;
    VKW::DescriptorSetLayout const* setLayout = pipelineLayout->GetMember(targetSet);

    m_DescriptorSet = device->GetDescriptorManager()->AllocatePerFrameSet(*setLayout, frameID);
}

void ResourceBinder::AddSampledTexture(DRE::U32 binding, Texture* texture)
{
    m_WriteDesc.AddSampledImage(texture->GetShaderView(), binding);
}

void ResourceBinder::AddStorageTexture(DRE::U32 binding, Texture* texture)
{
    m_WriteDesc.AddStorageImage(texture->GetShaderView(), binding);
}

void ResourceBinder::AddStorageBuffer(DRE::U32 binding, StorageBuffer* buffer)
{
    m_WriteDesc.AddStorageBuffer(buffer->GetResource(), binding);
}

void ResourceBinder::AddUniform(DRE::U32 binding, UniformProxy* uniform)
{
    UniformArena::Allocation const& allocation = uniform->GetAllocation();
    m_WriteDesc.AddUniform(allocation.m_Buffer, allocation.m_OffsetInBuffer, allocation.m_Size, binding);
}

void ResourceBinder::FlushDescriptorWrites()
{
    m_Device->GetDescriptorManager()->WriteDescriptorSet(m_DescriptorSet, m_WriteDesc);
}

ResourceBinder::~ResourceBinder()
{
    FlushDescriptorWrites();
}

}
