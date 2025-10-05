#include <gfx\renderer\RenderableObject.hpp>

#include <glm\gtc\quaternion.hpp>

#include <gfx\texture\Texture.hpp>

namespace GFX
{

RenderableObject::RenderableObject(WORLD::SceneNode* sceneNode, InstanceDataManager::InstanceGPU const& instanceGPU,
    GlobalGeometry::GeometryGPU const& geometryGPU,
    VKW::AccelerationStructureResource* blasResource
)
    : m_SceneNode{ sceneNode }
    , m_LayerBits{ 0 }
    , m_Pipelines{}
    , m_BLASResource{ blasResource }
    , m_GeometryGPU{ geometryGPU }
    , m_InstanceGPU{ instanceGPU }
{
    for (DRE::U32 i = 0, size = DRE::U32(Layer::LAYER_COUNT); i < size; i++)
    {
        m_Pipelines.EmplaceBack(nullptr);
    }
}

void RenderableObject::AddLayerPipeline(Layer layer, VKW::Pipeline* pipeline)
{
    m_LayerBits |= LayerToBits(layer);
    m_Pipelines[DRE::U32(layer)] = pipeline;
}

void RenderableObject::SetInstanceFlags(InstanceFlags flags)
{
    m_InstanceGPU.ScheduleUpdate(flags);
}

void RenderableObject::SetFlag(InstanceFlags flag, bool enable)
{
    InstanceFlags cachedFlags = m_InstanceGPU.GetFlags();
    if (enable)
        cachedFlags = InstanceFlags(cachedFlags | flag);
    else
        cachedFlags = InstanceFlags(cachedFlags & ~flag);

    m_InstanceGPU.ScheduleUpdate(cachedFlags);
}

void RenderableObject::SetMaterialGPU(MaterialsManager::MaterialGPU const& materialGPU)
{
    m_InstanceGPU.ScheduleUpdate(materialGPU);
}

}