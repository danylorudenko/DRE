#include <gfx\renderer\RenderableObject.hpp>

#include <glm\gtc\quaternion.hpp>

namespace GFX
{

RenderableObject::RenderableObject(
    WORLD::SceneNode* sceneNode, InstanceDataManager::InstanceGPU const& instanceGPU, LayerBits layers, VKW::Pipeline* pipeline, GlobalGeometry::GeometryGPU const& geometryGPU,
    VKW::AccelerationStructureResource* blasResource,
    TexturesVector&& textures, DescriptorSetVector&& sets, DescriptorSetVector&& shadowSets)
    : m_SceneNode{ sceneNode }
    , m_Layer{ layers }
    , m_Pipeline{ pipeline }
    , m_BLASResource{ blasResource }
    , m_GeometryGPU{ geometryGPU }
    , m_InstanceGPU{ instanceGPU }
    , m_DescriptorSets{ DRE_MOVE(sets) }
    , m_DescriptorSetsShadow{ DRE_MOVE(shadowSets) }
    , m_Textures{ DRE_MOVE(textures) }
{
}

RenderableObject::RenderableObject(
    WORLD::SceneNode* sceneNode, InstanceDataManager::InstanceGPU const& instanceGPU, LayerBits layers, VKW::Pipeline* pipeline, GlobalGeometry::GeometryGPU const& geometryGPU,
    VKW::AccelerationStructureResource* blasResource,
    DescriptorSetVector&& sets, DescriptorSetVector&& shadowSets)
    : m_SceneNode{ sceneNode }
    , m_Layer{ layers }
    , m_Pipeline{ pipeline }
    , m_BLASResource{ blasResource }
    , m_GeometryGPU{ geometryGPU }
    , m_InstanceGPU{ instanceGPU }
    , m_DescriptorSets{ DRE_MOVE(sets) }
    , m_DescriptorSetsShadow{ DRE_MOVE(shadowSets) }
    , m_Textures{}
{
}

RenderableObject::RenderableObject(WORLD::SceneNode* sceneNode, InstanceDataManager::InstanceGPU const& instanceGPU, LayerBits layers, VKW::Pipeline* pipeline,
    GlobalGeometry::GeometryGPU const& geometryGPU,
    VKW::AccelerationStructureResource* blasResource)
    : m_SceneNode{ sceneNode }
    , m_Layer{ layers }
    , m_Pipeline{ pipeline }
    , m_BLASResource{ blasResource }
    , m_GeometryGPU{ geometryGPU }
    , m_InstanceGPU{ instanceGPU }
    , m_DescriptorSets{}
    , m_DescriptorSetsShadow{}
    , m_Textures{}
{
}

}