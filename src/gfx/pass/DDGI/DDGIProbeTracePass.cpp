#include <gfx\pass\DDGI\DDGIProbeTracePass.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>

namespace GFX
{

PassID DDGIProbeTracePass::GetID() const
{
    return PassID::DDGIProbeTrace;
}

void DDGIProbeTracePass::RegisterResources(RenderGraph& graph)
{
}

void DDGIProbeTracePass::Initialize(RenderGraph& graph)
{
}

void DDGIProbeTracePass::Render(RenderGraph& graph, VKW::Context& context)
{
}

}