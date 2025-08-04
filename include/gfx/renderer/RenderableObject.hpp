#pragma once

#include <glm\mat4x4.hpp>
#include <glm\vec3.hpp>

#include <foundation\container\InplaceVector.hpp>

#include <vk_wrapper\Constant.hpp>
#include <vk_wrapper\descriptor\Descriptor.hpp>

#include <gfx\FrameID.hpp>
#include <gfx\renderer\GlobalGeometryManager.hpp>
#include <gfx\renderer\InstanceDataManager.hpp>

#include <engine\data\Material.hpp>

namespace VKW
{
class Pipeline;
struct BufferResource;
}

namespace WORLD
{
class SceneNode;
}

namespace GFX
{

class Texture;

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
    using TexturesVector      = DRE::InplaceVector<Texture*, Data::Material::TextureProperty::Slot::MAX>;

    RenderableObject(
        WORLD::SceneNode* sceneNode, InstanceDataManager::InstanceGPU const& instanceGPU, LayerBits layers, VKW::Pipeline* pipeline,
        GlobalGeometry::GeometryGPU const& geometryGPU,
        VKW::AccelerationStructureResource* blasResource,
        TexturesVector&& textures, DescriptorSetVector&& sets, DescriptorSetVector&& shadowSets,
        InstanceFlags instanceFlags);

    RenderableObject(
        WORLD::SceneNode* sceneNode, InstanceDataManager::InstanceGPU const& instanceGPU, LayerBits layers, VKW::Pipeline* pipeline,
        GlobalGeometry::GeometryGPU const& goemtryGPU,
        VKW::AccelerationStructureResource* blasResource,
        DescriptorSetVector&& sets, DescriptorSetVector&& shadowSets,
        InstanceFlags instanceFlags);

    RenderableObject(
        WORLD::SceneNode* sceneNode, InstanceDataManager::InstanceGPU const& instanceGPU, LayerBits layers, VKW::Pipeline* pipeline,
        GlobalGeometry::GeometryGPU const& geometryGPU,
        VKW::AccelerationStructureResource* blasResource,
        InstanceFlags instanceFlags);

    inline WORLD::SceneNode*                    GetSceneNode() const { return m_SceneNode; }
    inline LayerBits                            GetLayer() const { return m_Layer; }
    inline VKW::Pipeline*                       GetPipeline() const{ return m_Pipeline; }
    inline VKW::AccelerationStructureResource*  GetBLASResource() const { return m_BLASResource; }
    inline InstanceDataManager::InstanceGPU&    GetInstanceGPU() { return m_InstanceGPU; }
    inline GlobalGeometry::GeometryGPU&         GetGeometryGPU() { return m_GeometryGPU; }
    inline bool                                 HasDescriptorSet() const { return !m_DescriptorSets.Empty(); }
    inline VKW::DescriptorSet const&            GetDescriptorSet(FrameID frameID) const { return m_DescriptorSets[frameID]; }
    inline VKW::DescriptorSet const&            GetShadowDescriptorSet(FrameID frameID) const { return m_DescriptorSetsShadow[frameID]; }
    inline Texture*                             GetDiffuseTexture() const { return m_Textures[Data::Material::TextureProperty::DIFFUSE]; }
    inline Texture*                             GetNormalTexture() const { return m_Textures[Data::Material::TextureProperty::NORMAL]; }
    inline Texture*                             GetMetalnessTexture() const { return m_Textures[Data::Material::TextureProperty::METALNESS]; }
    inline Texture*                             GetRoughnessTexture() const { return m_Textures[Data::Material::TextureProperty::ROUGHNESS]; }
    inline InstanceFlags                        GetInstanceFlags() const { return m_InstanceFlags; }

    void                                        SetDiffuseTexture(Texture* texture);
    void                                        SetNormalTexture(Texture* texture);
    void                                        SetMetalnessTexture(Texture* texture);
    void                                        SetRoughnessTexture(Texture* texture);

    void                                        SetInstanceFlags(InstanceFlags flags);
    void                                        SetNormalTexture(bool enable);
    void                                        SetNormalTextureInvertY(bool enable);
    void                                        SetNormalTBN(bool enable);
    void                                        SetMaterialTexturesDefault(bool enable);
    void                                        SetMaterialTexturesGLTFSpheres(bool enable);

private:
    void                                        UpdateGPUInstanceTextures();
    void                                        SetFlag(InstanceFlags flag, bool enable);

private:
    WORLD::SceneNode*                   m_SceneNode;
    LayerBits                           m_Layer;
    VKW::Pipeline*                      m_Pipeline;
    VKW::AccelerationStructureResource* m_BLASResource;

    GlobalGeometry::GeometryGPU         m_GeometryGPU;
    InstanceDataManager::InstanceGPU    m_InstanceGPU;

    TexturesVector                      m_Textures;

    InstanceFlags                       m_InstanceFlags;

    DescriptorSetVector                 m_DescriptorSets;
    DescriptorSetVector                 m_DescriptorSetsShadow;
};

}
