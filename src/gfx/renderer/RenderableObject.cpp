#include <gfx\renderer\RenderableObject.hpp>

#include <glm\gtc\quaternion.hpp>

namespace GFX
{

RenderableObject::RenderableObject(LayerBits layers, VKW::Pipeline* pipeline, VKW::BufferResource* vertexBuffer, std::uint32_t vertexCount, VKW::BufferResource* indexBuffer, std::uint32_t indexCount, TexturesVector&& textures, DescriptorSetVector&& sets)
    : m_Layer{ layers }
    , m_ModelM{ glm::identity<glm::mat4>() }
    , m_PrevModelM{ glm::identity<glm::mat4>() }
    , m_Pipeline{ pipeline }
    , m_VertexBuffer{ vertexBuffer }
    , m_IndexBuffer{ indexBuffer }
    , m_VertexCount{ vertexCount }
    , m_IndexCount{ indexCount }
    , m_DescriptorSets{ DRE_MOVE(sets) }
    , m_Textures{ DRE_MOVE(textures) }
{
}

RenderableObject::RenderableObject(LayerBits layers, VKW::Pipeline* pipeline, VKW::BufferResource* vertexBuffer, std::uint32_t vertexCount, VKW::BufferResource* indexBuffer, std::uint32_t indexCount, DescriptorSetVector&& sets)
    : m_Layer{ layers } 
    , m_ModelM{ glm::identity<glm::mat4>() }
    , m_PrevModelM{ glm::identity<glm::mat4>() }
    , m_Pipeline{ pipeline }
    , m_VertexBuffer{ vertexBuffer }
    , m_IndexBuffer{ indexBuffer }
    , m_VertexCount{ vertexCount }
    , m_IndexCount{ indexCount }
    , m_DescriptorSets{ DRE_MOVE(sets) }
    , m_Textures{}
{
}

RenderableObject::RenderableObject(LayerBits layers, VKW::Pipeline* pipeline, VKW::BufferResource* vertexBuffer, std::uint32_t vertexCount, VKW::BufferResource* indexBuffer, std::uint32_t indexCount)
    : m_Layer{ layers } 
    , m_ModelM{ glm::identity<glm::mat4>() }
    , m_PrevModelM{ glm::identity<glm::mat4>() }
    , m_Pipeline{ pipeline }
    , m_VertexBuffer{ vertexBuffer }
    , m_IndexBuffer{ indexBuffer }
    , m_VertexCount{ vertexCount }
    , m_IndexCount{ indexCount }
    , m_DescriptorSets{}
    , m_Textures{}
{
}

void RenderableObject::Transform(glm::mat4 model)
{
    m_PrevModelM = m_ModelM;
    m_ModelM = model;
}

}