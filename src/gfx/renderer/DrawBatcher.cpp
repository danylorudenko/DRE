#include <gfx\renderer\DrawBatcher.hpp>

#include <vk_wrapper\descriptor\DescriptorManager.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\renderer\RenderableObject.hpp>
#include <gfx\view\RenderView.hpp>

#include <engine/scene/Scene.hpp>

#include <glm\gtc\matrix_transform.hpp>

namespace GFX
{

DrawBatcher::DrawBatcher(DRE::AllocatorLinear* allocator, VKW::DescriptorManager* descriptorManager, UniformArena* uniformArena)
    : m_Allocator{ allocator }
    , m_DescriptorManager{ descriptorManager }
    , m_UniformArena{ uniformArena }
    , m_OpaqueDraws{ allocator }
{
}

void DrawBatcher::Batch(VKW::Context& context, RenderView const& view, AtomDataDelegate atomDelegate)
{
    auto const& renderables = view.GetObjects();
    for (std::uint32_t i = 0, count = renderables.Size(); i < count; i++)
    {
        RenderableObject& obj = *renderables[i];

        atomDelegate(obj, context, *m_DescriptorManager, *m_UniformArena, view);

        AtomDraw& atom = m_OpaqueDraws.EmplaceBack();
        atom.vertexBuffer  = obj.GetVertexBuffer();
        atom.vertexOffset  = 0;
        atom.vertexCount   = obj.GetVertexCount();

        atom.indexBuffer   = obj.GetIndexBuffer();
        atom.indexOffset   = 0;
        atom.indexCount    = obj.GetIndexCount();

        atom.pipeline      = obj.GetPipeline();
        atom.descriptorSet = obj.GetDescriptorSets(g_GraphicsManager->GetCurrentFrameID());

    }
}


}