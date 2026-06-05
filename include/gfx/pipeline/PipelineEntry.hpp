#pragma once

#include <foundation\String\InplaceString.hpp>
#include <foundation\container\InplaceVector.hpp>

#include <vk_wrapper\pipeline\Pipeline.hpp>

#include <engine\io\ShaderDB.hpp>

namespace IO
{
class ShaderEntry;
}

namespace GFX
{

///////////////////////////////////////
class PipelineEntry
{
public:
    using ShaderEntries = DRE::InplaceVector<IO::ShaderEntry const*, VKW::Pipeline::MAX_SHADER_STAGES>;

    PipelineEntry(VKW::Pipeline&& pipeline, VKW::PipelineLayout* layout, ShaderEntries&& shaderEntries, char const* name);

    VKW::Pipeline*        GetPipeline()      { return &m_Pipeline; }
    VKW::PipelineLayout*  GetLayout()        { return m_Layout; }
    ShaderEntries const&  GetShaderEntries() const { return m_ShaderEntries; }

    void SetPipeline(VKW::Pipeline&& newPipeline);

private:
    DRE::String128          m_Name;
    VKW::Pipeline           m_Pipeline;
    VKW::PipelineLayout*    m_Layout;
    ShaderEntries           m_ShaderEntries;
};

}
