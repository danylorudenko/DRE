#include <gfx\scheduling\GraphDescriptorManager.hpp>

#include <vk_wrapper\Device.hpp>
#include <vk_wrapper\pipeline\Dependency.hpp>

#include <gfx\buffer\StorageBuffer.hpp>
#include <gfx\texture\StorageTexture.hpp>
#include <gfx\scheduling\GraphResourcesManager.hpp>
#include <gfx\pipeline\PipelineDB.hpp>

namespace GFX
{

GraphDescriptorManager::GraphDescriptorManager(VKW::Device* device, GraphResourcesManager* resourcesManager, PipelineDB* pipelineDB)
    : DeviceChild{ device }
    , m_ResourcesManager{ resourcesManager }
    , m_PipelineDB{ pipelineDB }
{
}

void GraphDescriptorManager::RegisterTexture(PassID pass, TextureID texture, VKW::ResourceAccess access, VKW::DescriptorStage stages, std::uint8_t binding)
{
    SetInfo& setInfo = m_DescriptorsInfo[pass];
    setInfo.descriptorInfos.EmplaceBack(texture, access, stages, 0u, 0u, std::uint8_t(1), binding);
}

void GraphDescriptorManager::RegisterBuffer(PassID pass, BufferID buffer, VKW::ResourceAccess access, VKW::DescriptorStage stages, std::uint8_t binding)
{
    SetInfo& setInfo = m_DescriptorsInfo[pass];
    setInfo.descriptorInfos.EmplaceBack(buffer, access, stages, 0u, 0u, std::uint8_t(0), binding);
}

void GraphDescriptorManager::RegisterUniformBuffer(PassID pass, UniformArena::Allocation& allocation, VKW::DescriptorStage stages, std::uint8_t binding)
{
    // onlny one uniform buffer is allowed per pass
    SetInfo& setInfo = m_DescriptorsInfo[pass];
    setInfo.descriptorInfos.EmplaceBack(allocation.m_Buffer, VKW::ResourceAccess{ VKW::RESOURCE_ACCESS_SHADER_UNIFORM }, stages, allocation.m_OffsetInBuffer, allocation.m_Size, std::uint8_t(0), binding);
}

void GraphDescriptorManager::ResisterPushConstant(PassID pass, std::uint32_t size, VKW::DescriptorStage stage)
{
    SetInfo& setInfo = m_DescriptorsInfo[pass];
    setInfo.pushConstantSize = size;
    setInfo.pushConstantStages = stage;
}

void GraphDescriptorManager::InitDescriptors()
{
    m_PassDescriptors.Clear();

    VKW::ImportTable* table = m_ParentDevice->GetFuncTable();
    VKW::LogicalDevice* device = m_ParentDevice->GetLogicalDevice();
    VKW::DescriptorManager* allocator = m_ParentDevice->GetDescriptorAllocator();
    m_DescriptorsInfo.ForEach([table, device, allocator, this](auto const& pair)
    {
        SetInfo& setInfo = *pair.value;

        auto& descriptorInfos = setInfo.descriptorInfos;
        descriptorInfos.SortBubble([](DescriptorInfo const& lhs, DescriptorInfo const& rhs) { return lhs.m_Binding < rhs.m_Binding; });

        VKW::DescriptorStage stages = VKW::DESCRIPTOR_STAGE_NONE;
        for (std::uint8_t i = 0, size = descriptorInfos.Size(); i < size; i++) { stages |= descriptorInfos[i].m_Stages; } // collect all stages

        VKW::StandaloneDescriptorSet::Descriptor desc{/* stages */};
        for (std::uint8_t i = 0, size = descriptorInfos.Size(); i < size; i++)
        {
            DescriptorInfo const& info = descriptorInfos[i];
            if (info.m_IsTexture)
            {
                switch (info.m_Access)
                {
                case VKW::RESOURCE_ACCESS_SHADER_WRITE:
                case VKW::RESOURCE_ACCESS_SHADER_RW:
                case VKW::RESOURCE_ACCESS_SHADER_READ:
                    desc.AddStorageImage(m_ResourcesManager->GetStorageTexture(info.mu_TextureID)->GetShaderView(), info.m_Binding);
                    break;
                case VKW::RESOURCE_ACCESS_SHADER_SAMPLE:
                    desc.AddReadonlyImage(m_ResourcesManager->GetStorageTexture(info.mu_TextureID)->GetShaderView(), info.m_Binding);
                    break;
                }
            }
            else
            {
                if (info.m_Access == VKW::RESOURCE_ACCESS_SHADER_UNIFORM)
                {
                    desc.AddUniform(info.mu_UniformBuffer, info.m_Size0, info.m_Size1, info.m_Binding);
                }
                else
                {
                    desc.AddStorageBuffer(m_ResourcesManager->GetStorageBuffer(info.mu_BufferID)->GetResource(), info.m_Binding);
                }
            }
        }

        PerPassDescriptors& perPassDescriptors = m_PassDescriptors[*pair.key];
        perPassDescriptors.m_DescriptorSet = VKW::StandaloneDescriptorSet{ table, device, desc, allocator };

        VKW::PipelineLayout::Descriptor layoutDesc;
        m_PipelineDB->AddGlobalLayouts(layoutDesc);
        layoutDesc.Add(&perPassDescriptors.m_DescriptorSet.GetLayout());

        if (setInfo.pushConstantSize > 0)
        {
            layoutDesc.AddPushConstant(setInfo.pushConstantSize, setInfo.pushConstantStages);
        }

        char layoutName[16];
        std::sprintf(layoutName, "pass_layout_%i", int(*pair.key));
        
        perPassDescriptors.m_PipelineLayout = m_PipelineDB->CreatePipelineLayout(layoutName, layoutDesc);
    });
}

VKW::StandaloneDescriptorSet& GraphDescriptorManager::GetPassDescriptorSet(PassID pass)
{
    return m_PassDescriptors[pass].m_DescriptorSet;
}

VKW::PipelineLayout* GraphDescriptorManager::GetPassPipelineLayout(PassID pass)
{
    return m_PassDescriptors[pass].m_PipelineLayout;
}


}