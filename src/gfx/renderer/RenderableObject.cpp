#include <gfx\renderer\RenderableObject.hpp>

#include <glm\gtc\quaternion.hpp>

namespace GFX
{

RenderableObject::RenderableObject(
    WORLD::SceneNode* sceneNode, TransformsManager::TransformGPU transform, LayerBits layers, VKW::Pipeline* pipeline, VKW::BufferResource* vertexBuffer, std::uint32_t vertexCount,
    VKW::BufferResource* indexBuffer, std::uint32_t indexCount,
    VKW::AccelerationStructureResource* blasResource,
    TexturesVector&& textures, DescriptorSetVector&& sets, DescriptorSetVector&& shadowSets)
    : m_SceneNode{ sceneNode }
    , m_Layer{ layers }
    , m_Pipeline{ pipeline }
    , m_VertexBuffer{ vertexBuffer }
    , m_IndexBuffer{ indexBuffer }
    , m_BLASResource{ blasResource }
    , m_VertexCount{ vertexCount }
    , m_IndexCount{ indexCount }
    , m_Transform{ transform }
    , m_DescriptorSets{ DRE_MOVE(sets) }
    , m_DescriptorSetsShadow{ DRE_MOVE(shadowSets) }
    , m_Textures{ DRE_MOVE(textures) }
{
}

RenderableObject::RenderableObject(
    WORLD::SceneNode* sceneNode, TransformsManager::TransformGPU transform, LayerBits layers, VKW::Pipeline* pipeline, VKW::BufferResource* vertexBuffer, std::uint32_t vertexCount,
    VKW::BufferResource* indexBuffer, std::uint32_t indexCount,
    VKW::AccelerationStructureResource* blasResource,
    DescriptorSetVector&& sets, DescriptorSetVector&& shadowSets)
    : m_SceneNode{ sceneNode }
    , m_Layer{ layers }
    , m_Pipeline{ pipeline }
    , m_VertexBuffer{ vertexBuffer }
    , m_IndexBuffer{ indexBuffer }
    , m_BLASResource{ blasResource }
    , m_VertexCount{ vertexCount }
    , m_IndexCount{ indexCount }
    , m_Transform{ transform }
    , m_DescriptorSets{ DRE_MOVE(sets) }
    , m_DescriptorSetsShadow{ DRE_MOVE(shadowSets) }
    , m_Textures{}
{
}

RenderableObject::RenderableObject(WORLD::SceneNode* sceneNode, TransformsManager::TransformGPU transform, LayerBits layers, VKW::Pipeline* pipeline,
    VKW::BufferResource* vertexBuffer, std::uint32_t vertexCount, VKW::BufferResource* indexBuffer, std::uint32_t indexCount,
    VKW::AccelerationStructureResource* blasResource)
    : m_SceneNode{ sceneNode }
    , m_Layer{ layers }
    , m_Pipeline{ pipeline }
    , m_VertexBuffer{ vertexBuffer }
    , m_IndexBuffer{ indexBuffer }
    , m_BLASResource{ blasResource }
    , m_VertexCount{ vertexCount }
    , m_IndexCount{ indexCount }
    , m_Transform{ transform }
    , m_DescriptorSets{}
    , m_DescriptorSetsShadow{}
    , m_Textures{}
{
}

}