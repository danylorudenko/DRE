#include <gfx\renderer\RenderableObject.hpp>

#include <glm\gtc\quaternion.hpp>

namespace GFX
{

RenderableObject::RenderableObject(
    WORLD::SceneNode* sceneNode, LayerBits layers, VKW::Pipeline* pipeline, VKW::BufferResource* vertexBuffer, std::uint32_t vertexCount,
    VKW::BufferResource* indexBuffer, std::uint32_t indexCount,
    TexturesVector&& textures, DescriptorSetVector&& sets, DescriptorSetVector&& shadowSets)
    : m_SceneNode{ sceneNode }
    , m_Layer{ layers }
    , m_Pipeline{ pipeline }
    , m_VertexBuffer{ vertexBuffer }
    , m_IndexBuffer{ indexBuffer }
    , m_VertexCount{ vertexCount }
    , m_IndexCount{ indexCount }
    , m_DescriptorSets{ DRE_MOVE(sets) }
    , m_DescriptorSetsShadow{ DRE_MOVE(shadowSets) }
    , m_Textures{ DRE_MOVE(textures) }
{
}

RenderableObject::RenderableObject(
    WORLD::SceneNode* sceneNode, LayerBits layers, VKW::Pipeline* pipeline, VKW::BufferResource* vertexBuffer, std::uint32_t vertexCount,
    VKW::BufferResource* indexBuffer, std::uint32_t indexCount,
    DescriptorSetVector&& sets, DescriptorSetVector&& shadowSets)
    : m_SceneNode{ sceneNode }
    , m_Layer{ layers }
    , m_Pipeline{ pipeline }
    , m_VertexBuffer{ vertexBuffer }
    , m_IndexBuffer{ indexBuffer }
    , m_VertexCount{ vertexCount }
    , m_IndexCount{ indexCount }
    , m_DescriptorSets{ DRE_MOVE(sets) }
    , m_DescriptorSetsShadow{ DRE_MOVE(shadowSets) }
    , m_Textures{}
{
}

RenderableObject::RenderableObject(WORLD::SceneNode* sceneNode, LayerBits layers, VKW::Pipeline* pipeline,
    VKW::BufferResource* vertexBuffer, std::uint32_t vertexCount, VKW::BufferResource* indexBuffer, std::uint32_t indexCount)
    : m_SceneNode{ sceneNode }
    , m_Layer{ layers }
    , m_Pipeline{ pipeline }
    , m_VertexBuffer{ vertexBuffer }
    , m_IndexBuffer{ indexBuffer }
    , m_VertexCount{ vertexCount }
    , m_IndexCount{ indexCount }
    , m_DescriptorSets{}
    , m_DescriptorSetsShadow{}
    , m_Textures{}
{
}

}