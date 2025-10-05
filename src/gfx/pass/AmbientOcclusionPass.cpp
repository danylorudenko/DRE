#include <gfx\pass\AmbientOcclusionPass.hpp>

#include <gfx\scheduling\RenderGraph.hpp>

namespace GFX
{

PassID AmbientOcclusionPass::GetID() const
{
    return PassID::AmbientOcclusion;
}

void AmbientOcclusionPass::Initialize(RenderGraph& graph)
{
}

void AmbientOcclusionPass::RegisterResources(RenderGraph& graph)
{
}

void AmbientOcclusionPass::Render(RenderGraph& graph, VKW::Context& context)
{
}

}
