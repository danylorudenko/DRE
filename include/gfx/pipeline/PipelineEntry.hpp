#pragma once

#include <foundation\String\InplaceString.hpp>

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
    PipelineEntry(VKW::Pipeline&& pipeline, VKW::PipelineLayout&& layout, IO::ShaderEntry* shaderEntry, char const* name);

    VKW::Pipeline* GetPipeline() { return &pipeline; }

private:
    DRE::String128          name;
    VKW::Pipeline           pipeline;
    VKW::PipelineLayout     layout;
    IO::ShaderEntry*        shaderEntry;
};

}
