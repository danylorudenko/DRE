#pragma once

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>
#include <gfx\texture\Texture.hpp>
#include <gfx\buffer\StorageBuffer.hpp>
#include <gfx\buffer\UniformProxy.hpp>
#include <gfx\pipeline\PipelineEntry.hpp>
#include <vk_wrapper\descriptor\DescriptorManager.hpp>

namespace GFX
{

// This class doesn't own any resources, everything passed to it must live throughout its lifetime.
class ResourceBinder
    : public NonCopyable
    , public NonMovable
{
public:
    ResourceBinder(VKW::Device* device, PipelineEntry* pipelineEntry, DRE::U32 setIdAfterGlobalSets, FrameID frameID);
    ~ResourceBinder();

    void AddSampledTexture      (DRE::U32 binding, Texture* texture);
    void AddStorageTexture      (DRE::U32 binding, Texture* texture);
    void AddStorageBuffer       (DRE::U32 binding, StorageBuffer* buffer);
    void AddUniform             (DRE::U32 binding, UniformProxy* uniform);

    void FlushDescriptorWrites();

private:
    PipelineEntry*                      m_PipelineEntry;
    VKW::Device*                        m_Device;

    VKW::DescriptorManager::WriteDesc   m_WriteDesc;
    VKW::DescriptorSet                  m_DescriptorSet;
};

}
