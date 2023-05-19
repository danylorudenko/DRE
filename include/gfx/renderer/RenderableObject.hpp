#pragma once

#include <glm\mat4x4.hpp>
#include <glm\vec3.hpp>

#include <foundation\container\InplaceVector.hpp>

#include <vk_wrapper\Constant.hpp>
#include <vk_wrapper\descriptor\Descriptor.hpp>

#include <gfx\FrameID.hpp>

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
    enum LayerBits
    {
        LAYER_NONE              = 0,
        LAYER_OPAQUE_BIT        = 1 << 0,
        LAYER_WATER_BIT         = 1 << 1
    };

    using DescriptorSetVector = DRE::InplaceVector<VKW::DescriptorSet, VKW::CONSTANTS::FRAMES_BUFFERING>;
    using TexturesVector      = DRE::InplaceVector<ReadOnlyTexture*, Data::Material::TextureProperty::Slot::MAX>;

    RenderableObject(LayerBits layers, VKW::Pipeline* pipeline, VKW::BufferResource* vertexBuffer, std::uint32_t vertexCount, VKW::BufferResource* indexBuffer, std::uint32_t indexCount, TexturesVector&& textures, DescriptorSetVector&& sets);
    RenderableObject(LayerBits layers, VKW::Pipeline* pipeline, VKW::BufferResource* vertexBuffer, std::uint32_t vertexCount, VKW::BufferResource* indexBuffer, std::uint32_t indexCount, DescriptorSetVector&& sets);
    RenderableObject(LayerBits layers, VKW::Pipeline* pipeline, VKW::BufferResource* vertexBuffer, std::uint32_t vertexCount, VKW::BufferResource* indexBuffer, std::uint32_t indexCount);

    inline LayerBits                    GetLayer() const { return m_Layer; }
    inline glm::mat4x4 const&           GetModelM() const { return m_ModelM; }
    inline glm::mat4x4 const&           GetPrevModelM() const { return m_PrevModelM; }
    inline VKW::Pipeline*               GetPipeline() const{ return m_Pipeline; }
    inline VKW::BufferResource*         GetVertexBuffer() const{ return m_VertexBuffer; }
    inline VKW::BufferResource*         GetIndexBuffer() const { return m_IndexBuffer; }
    inline std::uint32_t                GetVertexCount() const { return m_VertexCount; }
    inline std::uint32_t                GetIndexCount() const { return m_IndexCount; }
    inline VKW::DescriptorSet const&    GetDescriptorSet(FrameID frameID) const { return m_DescriptorSets[frameID]; }
    inline ReadOnlyTexture*             GetDiffuseTexture() const { return m_Textures[0]; }
    inline ReadOnlyTexture*             GetNormalTexture() const { return m_Textures[1]; }
    inline ReadOnlyTexture*             GetMetalnessTexture() const { return m_Textures[2]; }
    inline ReadOnlyTexture*             GetRoughnessTexture() const { return m_Textures[3]; }

    void                                Transform(glm::mat4 model);

private:
    LayerBits               m_Layer;
    glm::mat4x4             m_ModelM;
    glm::mat4x4             m_PrevModelM;
    VKW::Pipeline*          m_Pipeline;
    VKW::BufferResource*    m_VertexBuffer;
    VKW::BufferResource*    m_IndexBuffer;

    std::uint32_t           m_VertexCount;
    std::uint32_t           m_IndexCount;

    TexturesVector          m_Textures;

    DescriptorSetVector     m_DescriptorSets;
};

}
