#include <gfx\renderer\DrawBatcher.hpp>

#include <vk_wrapper\descriptor\DescriptorManager.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\renderer\RenderableObject.hpp>

#include <engine/scene/Scene.hpp>

#include <glm\gtc\matrix_transform.hpp>

namespace GFX
{

DrawBatcher::DrawBatcher(DRE::AllocatorLinear* allocator, VKW::DescriptorManager* descriptorManager, UniformArena* uniformArena)
    : m_Allocator{ allocator }
    , m_DescriptorManager{ descriptorManager }
    , m_UniformArena{ uniformArena }
    , m_AllRenderables{ allocator }
    , m_OpaqueDraws{ allocator }
{
}

void DrawBatcher::AddRenderable(RenderableObject* renderable)
{
    m_AllRenderables.EmplaceBack(renderable);
}

void DrawBatcher::Batch(VKW::Context& context, WORLD::Scene const& scene)
{
    WORLD::Camera const& camera = scene.GetMainCamera();
    for (std::uint32_t i = 0, count = m_AllRenderables.Size(); i < count; i++)
    {
        RenderableObject& obj = *m_AllRenderables[i];

         std::uint32_t constexpr uniformSize = 
            sizeof(obj.GetModelM()) + 
            sizeof(VKW::TextureDescriptorIndex) * 4;

         auto uniformAllocation = m_UniformArena->AllocateTransientRegion(g_GraphicsManager->GetCurrentFrameID(), uniformSize, 256);
         VKW::DescriptorManager::WriteDesc writeDesc;
         writeDesc.AddUniform(uniformAllocation.m_Buffer, uniformAllocation.m_OffsetInBuffer, uniformAllocation.m_Size, 0);
         m_DescriptorManager->WriteDescriptorSet(obj.GetDescriptorSets(g_GraphicsManager->GetCurrentFrameID()), writeDesc);

         UniformProxy uniformProxy{ &context, uniformAllocation };

         glm::mat4 const mvp = camera.GetProjM() * camera.GetViewM() * obj.GetModelM();
         uniformProxy.WriteMember140(mvp);
         uniformProxy.WriteMember140(obj.GetModelM());

         std::uint32_t textureIDs[4] = { 
            obj.GetDiffuseTexture()->GetShaderReadDescriptor().id_, 
            obj.GetNormalTexture()->GetShaderReadDescriptor().id_,
            obj.GetMetalnessTexture()->GetShaderReadDescriptor().id_,
            obj.GetRoughnessTexture()->GetShaderReadDescriptor().id_
         };

         uniformProxy.WriteMember140(textureIDs, sizeof(textureIDs));

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