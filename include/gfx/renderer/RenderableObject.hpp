#pragma once

#include <glm\mat4x4.hpp>
#include <glm\vec3.hpp>

#include <foundation\container\InplaceVector.hpp>

#include <vk_wrapper\Constant.hpp>
#include <vk_wrapper\descriptor\Descriptor.hpp>

#include <engine\data\Material.hpp>

namespace VKW
{
class Pipeline;
struct BufferResource;
}


namespace GFX
{

class ReadOnlyTexture;

class RenderableObject
    : public NonCopyable
{
public:
    using DescriptorSetVector = DRE::InplaceVector<VKW::DescriptorSet, VKW::CONSTANTS::MAX_PIPELINE_LAYOUT_MEMBERS>;
    using TexturesVector      = DRE::InplaceVector<ReadOnlyTexture*, Data::Material::TextureProperty::Slot::MAX>;

    RenderableObject(VKW::Pipeline* pipeline, VKW::BufferResource* vertexBuffer, VKW::BufferResource* indexBuffer, TexturesVector&& textures, DescriptorSetVector&& sets);

    inline glm::mat4x4 const&           GetModelM() const { return m_ModelM; }
    inline VKW::Pipeline*               GetPipeline() const{ return m_Pipeline; }
    inline VKW::BufferResource*         GetVertexBuffer() const{ return m_VertexBuffer; }
    inline VKW::BufferResource*         GetIndexBuffer() const { return m_IndexBuffer; }
    inline DescriptorSetVector const&   GetDescriptorSets() const { return m_DescriptorSets; }

    void                                Transform(glm::mat4 model);

private:
    glm::mat4x4             m_ModelM;
    VKW::Pipeline*          m_Pipeline;
    VKW::BufferResource*    m_VertexBuffer;
    VKW::BufferResource*    m_IndexBuffer;

    TexturesVector          m_Textures;

    DescriptorSetVector     m_DescriptorSets;
};

}
