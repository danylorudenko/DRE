#pragma once

#include <foundation\class_features\NonMovable.hpp>

#include <foundation\Container\InplaceHashTable.hpp>
#include <foundation\Container\InplaceVector.hpp>
#include <foundation\string\InplaceString.hpp>

#include <vk_wrapper\pipeline\Dependency.hpp>

#include <gfx\DeviceChild.hpp>
#include <gfx\pass\PassID.hpp>
#include <gfx\scheduling\GraphResource.hpp>
#include <gfx\buffer\TransientArena.hpp>

namespace GFX
{

class StorageTexture;
class StorageBuffer;
class GraphResourcesManager;
class PipelineDB;

class GraphDescriptorManager
    : public DeviceChild
    , public NonMovable
{
public:
    GraphDescriptorManager(VKW::Device* device, GraphResourcesManager* resourcesManager, PipelineDB* pipelineDB);
    virtual ~GraphDescriptorManager() {}

    void RegisterTexture        (PassID pass, char const* id, VKW::ResourceAccess access, VKW::DescriptorStage stages, std::uint8_t binding);
    void RegisterBuffer         (PassID pass, char const* id,  VKW::ResourceAccess access, VKW::DescriptorStage stages, std::uint8_t binding);
    void RegisterUniformBuffer  (PassID pass, VKW::DescriptorStage stages, std::uint8_t binding);
    void RegisterPushConstant   (PassID pass, std::uint32_t size, VKW::DescriptorStage stages);

    void InitDescriptors();
    void DestroyDescriptors();

    VKW::DescriptorSet              GetPassDescriptorSet(PassID pass, FrameID frameID);
    VKW::PipelineLayout*            GetPassPipelineLayout(PassID pass);
    std::uint32_t                   GetPassUniformBinding(PassID pass);

private:
    struct DescriptorInfo
    {
        DescriptorInfo(char const* resourceID,  VKW::ResourceAccess access, VKW::DescriptorStage stages, std::uint32_t size0, std::uint32_t size1, std::uint8_t isTexture, std::uint8_t binding)
            : m_ResourceID{ resourceID }, m_Access{ access }, m_Stages{ stages }, m_Size0{ size0 }, m_Size1{ size1 }, m_IsTexture{ isTexture }, m_Binding{ binding } {}

        DescriptorInfo(VKW::DescriptorStage stages, std::uint8_t binding)
            : m_ResourceID{ "" }, m_Access{VKW::RESOURCE_ACCESS_SHADER_UNIFORM}, m_Stages{stages}, m_Size0{0}, m_Size1{0}, m_IsTexture{false}, m_Binding{binding} {}

        DRE::String32           m_ResourceID;
        VKW::ResourceAccess     m_Access;
        VKW::DescriptorStage    m_Stages;
        std::uint32_t           m_Size0;
        std::uint32_t           m_Size1;
        std::uint8_t            m_IsTexture : 1;
        std::uint8_t            m_Binding   : 7;
    };
    struct SetInfo
    {
        DRE::InplaceVector<DescriptorInfo, VKW::CONSTANTS::MAX_SET_LAYOUT_MEMBERS> descriptorInfos;
        std::uint32_t           uniformBinding      = DRE_U32_MAX;
        VKW::DescriptorStage    uniformStage        = VKW::DESCRIPTOR_STAGE_NONE;
        VKW::DescriptorStage    pushConstantStage   = VKW::DESCRIPTOR_STAGE_NONE;
        std::uint32_t           pushConstantSize    = DRE_U32_MAX;
    };

    DRE::InplaceHashTable<PassID, SetInfo> m_DescriptorsInfo;

private:
    struct PerPassDescriptors
    {
        VKW::DescriptorSetLayout*    m_DescriptorLayout = nullptr;
        VKW::DescriptorSet           m_DescriptorSet[VKW::CONSTANTS::FRAMES_BUFFERING];
        VKW::PipelineLayout*         m_PipelineLayout = nullptr;
        std::uint32_t                m_UniformBinding = DRE_U32_MAX;
    };


    GraphResourcesManager*      m_ResourcesManager;
    PipelineDB*                 m_PipelineDB;

    DRE::InplaceHashTable<PassID, PerPassDescriptors> m_PassDescriptors;
};

}

