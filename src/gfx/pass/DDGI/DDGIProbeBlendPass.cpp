#include <gfx\pass\DDGI\DDGIProbeBlendPass.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>

namespace GFX
{

PassID DDGIProbeBlendPass::GetID() const
{
    return PassID::DDGIProbeBlend;
}

void DDGIProbeBlendPass::RegisterResources(RenderGraph& graph)
{
}

void DDGIProbeBlendPass::Initialize(RenderGraph& graph)
{
}

void DDGIProbeBlendPass::Render(RenderGraph& graph, VKW::Context& context)
{
}

}