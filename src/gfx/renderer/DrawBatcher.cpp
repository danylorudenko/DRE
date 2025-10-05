#include <gfx\renderer\DrawBatcher.hpp>

#include <vk_wrapper\descriptor\DescriptorManager.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\view\RenderView.hpp>

#include <engine/scene/Scene.hpp>

#include <glm\gtc\matrix_transform.hpp>

namespace GFX
{

DrawBatcher::DrawBatcher(DRE::AllocatorLinear* allocator, VKW::DescriptorManager* descriptorManager, UniformArena* uniformArena)
    : m_Allocator{ allocator }
    , m_DescriptorManager{ descriptorManager }
    , m_UniformArena{ uniformArena }
    , m_Draws{ allocator }
{
}

void DrawBatcher::Batch(VKW::Context& context, RenderView const& view, VKW::PipelineLayout const* layout, RenderableObject::Layer layer, AtomDataDelegate atomDelegate)
{
    auto const& renderables = view.GetObjects();
    DRE::U32 const layerBits = RenderableObject::LayerToBits(layer);
    for (std::uint32_t i = 0, count = renderables.Size(); i < count; i++)
    {
        RenderableObject& obj = *renderables[i];

        if ((obj.GetLayerBits() & layerBits) == 0)
            continue;

        if (atomDelegate != nullptr)
            atomDelegate(obj, context, *m_DescriptorManager, *m_UniformArena, view, layout);

        AtomDraw& atom = m_Draws.EmplaceBack();
        atom.vertexBuffer  = obj.GetGeometryGPU().GetBuffer();
        atom.vertexOffset  = obj.GetGeometryGPU().GetVertexOffset();
        atom.vertexCount   = obj.GetGeometryGPU().GetVertexCount();

        atom.indexBuffer   = obj.GetGeometryGPU().GetBuffer();
        atom.indexOffset   = obj.GetGeometryGPU().GetIndexOffset();
        atom.indexCount    = obj.GetGeometryGPU().GetIndexCount();

        atom.instanceID    = obj.GetInstanceGPU().GetID();

        atom.pipeline      = obj.GetPipeline(layer);

    }
}

}