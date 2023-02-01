#pragma once

#include <foundation\class_features\NonMovable.hpp>
#include <foundation\class_features\NonCopyable.hpp>

#include <foundation\Container\InplaceHashTable.hpp>
#include <foundation\String\InplaceString.hpp>

#include <vk_wrapper\pipeline\Pipeline.hpp>
#include <vk_wrapper\descriptor\DescriptorLayout.hpp>

namespace VKW
{
class Device;
}

namespace IO
{
class IOManager;
}

namespace GFX
{

class PipelineDB
    : public NonCopyable
    , public NonMovable
{
public:
    PipelineDB(VKW::Device* device, IO::IOManager* ioManager);
    ~PipelineDB();

    void                    AddGlobalLayouts(VKW::PipelineLayout::Descriptor& descriptor);


    VKW::DescriptorSetLayout*   CreateDescriptorSetLayout(const char* name, VKW::DescriptorSetLayout::Descriptor const& desc);
    VKW::PipelineLayout*        CreatePipelineLayout(char const* name, VKW::PipelineLayout::Descriptor const& descriptor);
    VKW::Pipeline*              CreatePipeline(char const* name, VKW::Pipeline::Descriptor& descriptor);
    void                        CreateDefaultPipelines();

    VKW::PipelineLayout const*  GetGlobalLayout() const;
    VKW::PipelineLayout*        GetLayout(char const* name);
    VKW::Pipeline*              GetPipeline(char const* name);
    VKW::DescriptorSetLayout*   GetSetLayout(char const* name);

private:
    // will find all modules with same name before '.' symbol (like "wall.vert.spv" + "wall.frag.spv") and combine all their layouts into one with name "wall_layout"
    DRE::String128 const*    CreatePipelineLayoutFromShader(char const* shaderName);

    DRE::String128 const*    CreateMaterialPipeline(char const* name);

private:
    VKW::Device*        m_Device;
    IO::IOManager*      m_IOManager;

    VKW::PipelineLayout m_GlobalLayout;

    using SetLayoutMap = DRE::InplaceHashTable<DRE::String128, DRE::InplaceVector<VKW::DescriptorSetLayout, VKW::CONSTANTS::MAX_PIPELINE_LAYOUT_MEMBERS - 3>>;

    SetLayoutMap                                                m_SetLayouts;
    DRE::InplaceHashTable<DRE::String128, VKW::PipelineLayout>  m_PipelineLayouts;
    DRE::InplaceHashTable<DRE::String128, VKW::Pipeline>        m_Pipelines;
};

}

