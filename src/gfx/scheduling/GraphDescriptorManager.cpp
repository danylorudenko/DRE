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

void GraphDescriptorManager::RegisterUniformBuffer(PassID pass, VKW::DescriptorStage stages, std::uint8_t binding)
{
    // onlny one uniform buffer is allowed per pass
    SetInfo& setInfo = m_DescriptorsInfo[pass];
    setInfo.uniformBinding = binding;
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
    VKW::DescriptorManager* allocator = m_ParentDevice->GetDescriptorManager();
    m_DescriptorsInfo.ForEach([table, device, allocator, this](auto const& pair)
    {
        PerPassDescriptors& perPassDescriptors = m_PassDescriptors[*pair.key];
        SetInfo& setInfo = *pair.value;

        auto& descriptorInfos = setInfo.descriptorInfos;
        descriptorInfos.SortBubble([](DescriptorInfo const& lhs, DescriptorInfo const& rhs) { return lhs.m_Binding < rhs.m_Binding; });

        VKW::DescriptorSetLayout::Descriptor layoutDesc{};
        VKW::DescriptorManager::WriteDesc writeDesc{};
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
                    layoutDesc.Add(VKW::DESCRIPTOR_TYPE_STORAGE_IMAGE, info.m_Binding, info.m_Stages);
                    writeDesc.AddStorageImage(m_ResourcesManager->GetStorageTexture(info.mu_TextureID)->GetShaderView(), info.m_Binding);
                    break;
                case VKW::RESOURCE_ACCESS_SHADER_SAMPLE:
                    layoutDesc.Add(VKW::DESCRIPTOR_TYPE_TEXTURE, info.m_Binding, info.m_Stages);
                    writeDesc.AddSampledImage(m_ResourcesManager->GetStorageTexture(info.mu_TextureID)->GetShaderView(), info.m_Binding);
                    break;
                }
            }
            else
            {
                layoutDesc.Add(VKW::DESCRIPTOR_TYPE_STORAGE_BUFFER, info.m_Binding, info.m_Stages);
                writeDesc.AddStorageBuffer(m_ResourcesManager->GetStorageBuffer(info.mu_BufferID)->GetResource(), info.m_Binding);
            }
        }

        if (setInfo.uniformBinding != DRE_U32_MAX)
        {
            layoutDesc.Add(VKW::DESCRIPTOR_TYPE_UNIFORM_BUFFER, setInfo.uniformBinding, VKW::DESCRIPTOR_STAGE_ALL);
        }

        char name[32];
        std::sprintf(name, "pass_set_layout_%i", int(*pair.key));

        perPassDescriptors.m_DescriptorLayout = m_PipelineDB->CreateDescriptorSetLayout(name, layoutDesc);

        for (std::uint8_t i = 0; i < VKW::CONSTANTS::FRAMES_BUFFERING; i++)
        {
            perPassDescriptors.m_DescriptorSet[i] = m_ParentDevice->GetDescriptorManager()->AllocateStandaloneSet(*perPassDescriptors.m_DescriptorLayout);
        }

        VKW::PipelineLayout::Descriptor pipelinelayoutDesc;
        m_PipelineDB->AddGlobalLayouts(pipelinelayoutDesc);
        pipelinelayoutDesc.Add(perPassDescriptors.m_DescriptorLayout);

        if (setInfo.pushConstantSize > 0)
        {
            pipelinelayoutDesc.AddPushConstant(setInfo.pushConstantSize, setInfo.pushConstantStages);
        }

        std::sprintf(name, "pass_layout_%i", int(*pair.key));
        
        perPassDescriptors.m_PipelineLayout = m_PipelineDB->CreatePipelineLayout(name, pipelinelayoutDesc);
    });
}

VKW::DescriptorSet GraphDescriptorManager::GetPassDescriptorSet(PassID pass, FrameID id)
{
    return m_PassDescriptors[pass].m_DescriptorSet[id];
}

VKW::PipelineLayout* GraphDescriptorManager::GetPassPipelineLayout(PassID pass)
{
    return m_PassDescriptors[pass].m_PipelineLayout;
}

std::uint32_t GraphDescriptorManager::GetPassUniformBinding(PassID pass)
{
    DRE_ASSERT(m_PassDescriptors[pass].m_UniformBinding != DRE_U32_MAX, "No binding assigned for pass uniform.");
    return m_PassDescriptors[pass].m_UniformBinding;
}


}