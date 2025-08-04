#include <gfx\renderer\RenderableObject.hpp>

#include <glm\gtc\quaternion.hpp>

#include <gfx\texture\Texture.hpp>

namespace GFX
{

RenderableObject::RenderableObject(
    WORLD::SceneNode* sceneNode, InstanceDataManager::InstanceGPU const& instanceGPU, LayerBits layers, VKW::Pipeline* pipeline, GlobalGeometry::GeometryGPU const& geometryGPU,
    VKW::AccelerationStructureResource* blasResource,
    TexturesVector&& textures, DescriptorSetVector&& sets, DescriptorSetVector&& shadowSets,
    InstanceFlags instanceFlags)
    : m_SceneNode{ sceneNode }
    , m_Layer{ layers }
    , m_Pipeline{ pipeline }
    , m_BLASResource{ blasResource }
    , m_GeometryGPU{ geometryGPU }
    , m_InstanceGPU{ instanceGPU }
    , m_DescriptorSets{ DRE_MOVE(sets) }
    , m_DescriptorSetsShadow{ DRE_MOVE(shadowSets) }
    , m_Textures{ DRE_MOVE(textures) }
    , m_InstanceFlags{ instanceFlags }
{
}

RenderableObject::RenderableObject(
    WORLD::SceneNode* sceneNode, InstanceDataManager::InstanceGPU const& instanceGPU, LayerBits layers, VKW::Pipeline* pipeline, GlobalGeometry::GeometryGPU const& geometryGPU,
    VKW::AccelerationStructureResource* blasResource,
    DescriptorSetVector&& sets, DescriptorSetVector&& shadowSets,
    InstanceFlags instanceFlags)
    : m_SceneNode{ sceneNode }
    , m_Layer{ layers }
    , m_Pipeline{ pipeline }
    , m_BLASResource{ blasResource }
    , m_GeometryGPU{ geometryGPU }
    , m_InstanceGPU{ instanceGPU }
    , m_DescriptorSets{ DRE_MOVE(sets) }
    , m_DescriptorSetsShadow{ DRE_MOVE(shadowSets) }
    , m_Textures{}
    , m_InstanceFlags{ instanceFlags }
{
}

RenderableObject::RenderableObject(WORLD::SceneNode* sceneNode, InstanceDataManager::InstanceGPU const& instanceGPU, LayerBits layers, VKW::Pipeline* pipeline,
    GlobalGeometry::GeometryGPU const& geometryGPU,
    VKW::AccelerationStructureResource* blasResource,
    InstanceFlags instanceFlags)
    : m_SceneNode{ sceneNode }
    , m_Layer{ layers }
    , m_Pipeline{ pipeline }
    , m_BLASResource{ blasResource }
    , m_GeometryGPU{ geometryGPU }
    , m_InstanceGPU{ instanceGPU }
    , m_DescriptorSets{}
    , m_DescriptorSetsShadow{}
    , m_Textures{}
    , m_InstanceFlags{ instanceFlags }
{
}

void RenderableObject::SetDiffuseTexture(Texture* texture)
{
    DRE_ASSERT(m_Textures[Data::Material::TextureProperty::DIFFUSE] == nullptr, "Overriding textures is not supported");
    m_Textures[Data::Material::TextureProperty::DIFFUSE] = texture;

    UpdateGPUInstanceTextures();
}
void RenderableObject::SetNormalTexture(Texture* texture)
{
    DRE_ASSERT(m_Textures[Data::Material::TextureProperty::NORMAL] == nullptr, "Overriding textures is not supported");
    m_Textures[Data::Material::TextureProperty::NORMAL] = texture;
    UpdateGPUInstanceTextures();
}

void RenderableObject::SetMetalnessTexture(Texture* texture)
{
    DRE_ASSERT(m_Textures[Data::Material::TextureProperty::METALNESS] == nullptr, "Overriding textures is not supported");
    m_Textures[Data::Material::TextureProperty::METALNESS] = texture;
    UpdateGPUInstanceTextures();
}

void RenderableObject::SetRoughnessTexture(Texture* texture)
{
    DRE_ASSERT(m_Textures[Data::Material::TextureProperty::ROUGHNESS] == nullptr, "Overriding textures is not supported");
    m_Textures[Data::Material::TextureProperty::ROUGHNESS] = texture;
    UpdateGPUInstanceTextures();
}

void RenderableObject::SetInstanceFlags(InstanceFlags flags)
{
    m_InstanceFlags = flags;
    m_InstanceGPU.ScheduleUpdate(m_InstanceFlags);
}

void RenderableObject::SetNormalTexture(bool enable)
{
    SetFlag(InstanceFlags::NORMAL_TEXTURE, enable);
}

void RenderableObject::SetNormalTextureInvertY(bool enable)
{
    SetFlag(InstanceFlags::NORMAL_TEXTURE_INVERT_Y, enable);
}

void RenderableObject::SetNormalTBN(bool enable)
{
    SetFlag(InstanceFlags::NORMAL_TBN, enable);
}

void RenderableObject::SetMaterialTexturesDefault(bool enable)
{
    SetFlag(InstanceFlags::MATERIAL_TEXTURES_DEFAULT, enable);
}

void RenderableObject::SetMaterialTexturesGLTFSpheres(bool enable)
{
    SetFlag(InstanceFlags::MATERIAL_TEXTURES_GLTF_SPHERES, enable);
}

void RenderableObject::UpdateGPUInstanceTextures()
{
    m_InstanceGPU.ScheduleUpdate(
        glm::uvec4{
            GetDiffuseTexture()->GetShaderGlobalDescriptor().id_,
            GetNormalTexture()->GetShaderGlobalDescriptor().id_,
            GetMetalnessTexture()->GetShaderGlobalDescriptor().id_,
            GetRoughnessTexture()->GetShaderGlobalDescriptor().id_
        }
    );
}

void RenderableObject::SetFlag(InstanceFlags flag, bool enable)
{
    if (enable)
        m_InstanceFlags = InstanceFlags(m_InstanceFlags | flag);
    else
        m_InstanceFlags = InstanceFlags(m_InstanceFlags & ~flag);

    m_InstanceGPU.ScheduleUpdate(m_InstanceFlags);
}

}