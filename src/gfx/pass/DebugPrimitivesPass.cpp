#include <gfx\pass\DebugPrimitivesPass.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>

#include <debug_primitives.slang>

namespace GFX
{

PassID DebugPrimitivesPass::GetID() const
{
    return PassID::DebugPrimitives;
}

void DebugPrimitivesPass::Initialize(RenderGraph& graph)
{
}

void DebugPrimitivesPass::RegisterResources(RenderGraph& graph)
{
}

void DebugPrimitivesPass::Render(RenderGraph& graph, VKW::Context& context)
{
}

}
