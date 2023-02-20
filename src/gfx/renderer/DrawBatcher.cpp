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

void TestFunc(RenderableObject& obj, VKW::Context& context, VKW::DescriptorManager& descriptorManager, RenderView const& view, UniformArena& arena)
{
    std::uint32_t constexpr uniformSize =
        sizeof(glm::mat4) * 2 +
        sizeof(VKW::TextureDescriptorIndex) * 4;

    auto uniformAllocation = arena.AllocateTransientRegion(g_GraphicsManager->GetCurrentFrameID(), uniformSize, 256);
    VKW::DescriptorManager::WriteDesc writeDesc;
    writeDesc.AddUniform(uniformAllocation.m_Buffer, uniformAllocation.m_OffsetInBuffer, uniformAllocation.m_Size, 0);
    descriptorManager.WriteDescriptorSet(obj.GetDescriptorSets(g_GraphicsManager->GetCurrentFrameID()), writeDesc);

    UniformProxy uniformProxy{ &context, uniformAllocation };

    //how to fucking write shadow uniform

    glm::mat4 const mvp = view.GetViewProjectionM() * obj.GetModelM();
    uniformProxy.WriteMember140(obj.GetModelM());
    uniformProxy.WriteMember140(mvp);

    std::uint32_t textureIDs[4] = {
       obj.GetDiffuseTexture()->GetShaderReadDescriptor().id_,
       obj.GetNormalTexture()->GetShaderReadDescriptor().id_,
       obj.GetMetalnessTexture()->GetShaderReadDescriptor().id_,
       obj.GetRoughnessTexture()->GetShaderReadDescriptor().id_
    };

    uniformProxy.WriteMember140(textureIDs, sizeof(textureIDs));
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