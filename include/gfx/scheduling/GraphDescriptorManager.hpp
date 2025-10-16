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

    void RegisterTexture        (PassID pass, char const* id, VKW::ResourceAccess access, VKW::DescriptorStage stages, DRE::U8 binding);
    void RegisterBuffer         (PassID pass, char const* id,  VKW::ResourceAccess access, VKW::DescriptorStage stages, DRE::U8 binding);
    void RegisterUniformBuffer  (PassID pass, VKW::DescriptorStage stages, DRE::U8 binding);
    //void RegisterPushConstant   (PassID pass, DRE::U32 size, VKW::DescriptorStage stages);

    void InitDescriptors();
    void DestroyDescriptors();

    VKW::DescriptorSet              GetPassDescriptorSet(PassID pass, FrameID frameID);
    VKW::PipelineLayout*            GetPassPipelineLayout(PassID pass);
    DRE::U32                        GetPassUniformBinding(PassID pass);

private:
    struct DescriptorInfo
    {
        DescriptorInfo(char const* resourceID,  VKW::ResourceAccess access, VKW::DescriptorStage stages, DRE::U32 size0, DRE::U32 size1, DRE::U8 isTexture, DRE::U8 binding)
            : m_ResourceID{ resourceID }, m_Access{ access }, m_Stages{ stages }, m_Size0{ size0 }, m_Size1{ size1 }, m_IsTexture{ isTexture }, m_Binding{ binding } {}

        DescriptorInfo(VKW::DescriptorStage stages, DRE::U8 binding)
            : m_ResourceID{ "" }, m_Access{VKW::RESOURCE_ACCESS_SHADER_UNIFORM}, m_Stages{stages}, m_Size0{0}, m_Size1{0}, m_IsTexture{false}, m_Binding{binding} {}

        DRE::String64           m_ResourceID;
        VKW::ResourceAccess     m_Access;
        VKW::DescriptorStage    m_Stages;
        DRE::U32                m_Size0;
        DRE::U32                m_Size1;
        DRE::U8                 m_IsTexture : 1;
        DRE::U8                 m_Binding   : 7;
    };
    struct SetInfo
    {
        DRE::InplaceVector<DescriptorInfo, VKW::CONSTANTS::MAX_SET_LAYOUT_MEMBERS> descriptorInfos;
        DRE::U32                uniformBinding      = DRE_U32_MAX;
        VKW::DescriptorStage    uniformStage        = VKW::DESCRIPTOR_STAGE_NONE;
        VKW::DescriptorStage    pushConstantStage   = VKW::DESCRIPTOR_STAGE_NONE;
        DRE::U32                pushConstantSize    = DRE_U32_MAX;
    };

    DRE::InplaceHashTable<PassID, SetInfo> m_DescriptorsInfo;

private:
    struct PerPassDescriptors
    {
        VKW::DescriptorSetLayout*    m_DescriptorLayout = nullptr;
        VKW::DescriptorSet           m_DescriptorSet[VKW::CONSTANTS::FRAMES_BUFFERING];
        VKW::PipelineLayout*         m_PipelineLayout = nullptr;
        DRE::U32                     m_UniformBinding = DRE_U32_MAX;
    };


    GraphResourcesManager*      m_ResourcesManager;
    PipelineDB*                 m_PipelineDB;

    DRE::InplaceHashTable<PassID, PerPassDescriptors> m_PassDescriptors;
};

}

