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
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DebugDrawBuffer), sizeof(DebugDrawBuffer), VKW::RESOURCE_ACCESS_GENERIC_READ);
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::SortedDebugDrawBuffer), sizeof(DebugDrawBuffer), VKW::RESOURCE_ACCESS_GENERIC_RW);
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DebugDrawCounters), sizeof(DebugDrawCounters), VKW::RESOURCE_ACCESS_GENERIC_RW);
    graph.RegisterStorageBuffer(this, RESOURCE_ID(BufferID::DebugDrawIndirectArgs), sizeof(DrawIndexedIndirectCommand) * uint(DebugDrawCommandType::COUNT), VKW::RESOURCE_ACCESS_INDIRECT_ARGS);
}

void DebugPrimitivesPass::Render(RenderGraph& graph, VKW::Context& context)
{

}

}
