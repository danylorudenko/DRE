#include <gfx\pipeline\PipelineEntry.hpp>

namespace GFX
{

PipelineEntry::PipelineEntry(VKW::Pipeline&& pipeline, VKW::PipelineLayout&& layout, ShaderEntries&& shaderEntries, char const* name)
    : name{ name }
    , pipeline{ DRE_MOVE(pipeline) }
    , layout{ DRE_MOVE(layout) }
    , shaderEntries{ DRE_MOVE(shaderEntries) }
{}

void PipelineEntry::SetPipeline(VKW::Pipeline&& newPipeline)
{
    pipeline = DRE_MOVE(newPipeline);
}

}
