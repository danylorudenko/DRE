#include <gfx\pipeline\PipelineEntry.hpp>

namespace GFX
{

PipelineEntry::PipelineEntry(VKW::Pipeline&& pipeline, VKW::PipelineLayout&& layout, IO::ShaderEntry* shaderEntry, char const* name)
    : name{ name }
    , pipeline{ DRE_MOVE(pipeline) }
    , layout{ DRE_MOVE(layout) }
    , shaderEntry{ shaderEntry }
{}

}
