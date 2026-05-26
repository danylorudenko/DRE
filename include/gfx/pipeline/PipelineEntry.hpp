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

    PipelineEntry(VKW::Pipeline&& pipeline, VKW::PipelineLayout&& layout, ShaderEntries&& shaderEntries, char const* name);

    VKW::Pipeline*        GetPipeline()      { return &pipeline; }
    VKW::PipelineLayout*  GetLayout()        { return &layout; }
    ShaderEntries const&  GetShaderEntries() const { return shaderEntries; }

    void SetPipeline(VKW::Pipeline&& newPipeline);

private:
    DRE::String128          name;
    VKW::Pipeline           pipeline;
    VKW::PipelineLayout     layout;
    ShaderEntries           shaderEntries;
};

}
