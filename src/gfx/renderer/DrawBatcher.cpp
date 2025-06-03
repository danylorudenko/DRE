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

void DrawBatcher::Batch(VKW::Context& context, RenderView const& view, VKW::PipelineLayout const* layout, RenderableObject::LayerBits layers, AtomDataDelegate atomDelegate)
{
    auto const& renderables = view.GetObjects();
    for (std::uint32_t i = 0, count = renderables.Size(); i < count; i++)
    {
        RenderableObject& obj = *renderables[i];

        if ((obj.GetLayer() & layers) == 0)
            continue;

        atomDelegate(obj, context, *m_DescriptorManager, *m_UniformArena, view, layout);

        AtomDraw& atom = m_Draws.EmplaceBack();
        atom.vertexBuffer  = obj.GetGeometryGPU().GetBuffer();
        atom.vertexOffset  = obj.GetGeometryGPU().GetVertexOffset();
        atom.vertexCount   = obj.GetGeometryGPU().GetVertexCount();

        atom.indexBuffer   = obj.GetGeometryGPU().GetBuffer();
        atom.indexOffset   = obj.GetGeometryGPU().GetIndexOffset();
        atom.indexCount    = obj.GetGeometryGPU().GetIndexCount();

        atom.instanceID    = obj.GetInstanceGPU().GetID();

        atom.pipeline      = obj.GetPipeline();
        atom.descriptorSet = obj.GetDescriptorSet(g_GraphicsManager->GetCurrentFrameID());

    }
}

void DrawBatcher::BatchShadow(VKW::Context& context, RenderView const& view, VKW::PipelineLayout const* passLayout, RenderableObject::LayerBits layers, AtomDataDelegate atomDelegate)
{
    VKW::Pipeline* shadowGenericPipeline = g_GraphicsManager->GetPipelineDB().GetPipeline("forward_shadow");

    auto const& renderables = view.GetObjects();
    for (std::uint32_t i = 0, count = renderables.Size(); i < count; i++)
    {
        RenderableObject& obj = *renderables[i];

        if ((obj.GetLayer() & layers) == 0)
            continue;

        atomDelegate(obj, context, *m_DescriptorManager, *m_UniformArena, view, passLayout);

        AtomDraw& atom = m_Draws.EmplaceBack();
        atom.vertexBuffer  = obj.GetGeometryGPU().GetBuffer();
        atom.vertexOffset  = obj.GetGeometryGPU().GetVertexOffset();
        atom.vertexCount   = obj.GetGeometryGPU().GetVertexCount();

        atom.indexBuffer   = obj.GetGeometryGPU().GetBuffer();
        atom.indexOffset   = obj.GetGeometryGPU().GetIndexOffset();
        atom.indexCount    = obj.GetGeometryGPU().GetIndexCount();

        atom.instanceID    = obj.GetInstanceGPU().GetID();

        atom.pipeline      = shadowGenericPipeline;
        atom.descriptorSet = obj.GetShadowDescriptorSet(g_GraphicsManager->GetCurrentFrameID());

    }
}


}