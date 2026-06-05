#include <gfx\pipeline\PipelineEntry.hpp>

namespace GFX
{

PipelineEntry::PipelineEntry(VKW::Pipeline&& pipeline, VKW::PipelineLayout* layout, ShaderEntries&& shaderEntries, char const* name)
    : m_Name{ name }
    , m_Pipeline{ DRE_MOVE(pipeline) }
    , m_Layout{ layout }
    , m_ShaderEntries{ DRE_MOVE(shaderEntries) }
{}

void PipelineEntry::SetPipeline(VKW::Pipeline&& newPipeline)
{
    m_Pipeline = DRE_MOVE(newPipeline);
}

}
