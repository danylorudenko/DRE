#pragma once

#include <foundation\class_features\NonMovable.hpp>
#include <foundation\class_features\NonCopyable.hpp>

#include <foundation\Container\InplaceHashTable.hpp>
#include <foundation\String\InplaceString.hpp>

#include <vk_wrapper\pipeline\Pipeline.hpp>

namespace VKW
{
class Device;
}

namespace GFX
{

class PipelineDB
    : public NonCopyable
    , public NonMovable
{
public:
    PipelineDB(VKW::Device* device);
    ~PipelineDB();

    void                    AddGlobalLayouts(VKW::PipelineLayout::Descriptor& descriptor);

    VKW::PipelineLayout*    CreatePipelineLayout(char const* name, VKW::PipelineLayout::Descriptor const& descriptor);
    VKW::Pipeline*          CreatePipeline(char const* name, VKW::Pipeline::Descriptor& descriptor);

    VKW::PipelineLayout const*  GetGlobalLayout() const;
    VKW::PipelineLayout*        GetLayout(char const* name);
    VKW::Pipeline*              GetPipeline(char const* name);

private:
    VKW::Device*        m_Device;

    VKW::PipelineLayout m_GlobalLayout;

    DRE::InplaceHashTable<DRE::String128, VKW::PipelineLayout>  m_PipelineLayouts;
    DRE::InplaceHashTable<DRE::String128, VKW::Pipeline>        m_Pipelines;
};

}

