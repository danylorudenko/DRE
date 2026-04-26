#include <gfx\pass\DDGI\DDGIProbeLightingPass.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>

namespace GFX
{

PassID DDGIProbeLightingPass::GetID() const
{
    return PassID::DDGIProbeLighting;
}

void DDGIProbeLightingPass::RegisterResources(RenderGraph& graph)
{
}

void DDGIProbeLightingPass::Initialize(RenderGraph& graph)
{
}

void DDGIProbeLightingPass::Render(RenderGraph& graph, VKW::Context& context)
{
}

}