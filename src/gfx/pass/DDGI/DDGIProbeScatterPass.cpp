#include <gfx\pass\DDGI\DDGIProbeScatterPass.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>

namespace GFX
{

PassID DDGIProbeScatterPass::GetID() const
{
    return PassID::DDGIProbeScatter;
}

void DDGIProbeScatterPass::RegisterResources(RenderGraph& graph)
{
}

void DDGIProbeScatterPass::Initialize(RenderGraph& graph)
{
}

void DDGIProbeScatterPass::Render(RenderGraph& graph, VKW::Context& context)
{
}

}