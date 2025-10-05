#pragma once

#include <glm\mat4x4.hpp>
#include <glm\vec3.hpp>

#include <foundation\container\InplaceVector.hpp>

#include <vk_wrapper\Constant.hpp>
#include <vk_wrapper\descriptor\Descriptor.hpp>

#include <gfx\FrameID.hpp>
#include <gfx\renderer\GlobalGeometryManager.hpp>
#include <gfx\renderer\GPUInstanceManager.hpp>
#include <gfx\renderer\GPUMaterialsManager.hpp>

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
    enum Layer
    {
        LAYER_FORWARD = 0,
        LAYER_WATER,
        LAYER_GBUFFER,
        LAYER_SHADOW,
        LAYER_COUNT
    };


    RenderableObject(
        WORLD::SceneNode* sceneNode, InstanceDataManager::InstanceGPU const& instanceGPU,
        GlobalGeometry::GeometryGPU const& goemtryGPU,
        VKW::AccelerationStructureResource* blasResource
    );

    inline WORLD::SceneNode*                    GetSceneNode() const { return m_SceneNode; }
    inline DRE::U32                             GetLayerBits() const { return m_LayerBits; }
    inline VKW::Pipeline*                       GetPipeline(Layer layer) { return m_Pipelines[layer]; }
    inline VKW::AccelerationStructureResource*  GetBLASResource() const { return m_BLASResource; }
    inline InstanceDataManager::InstanceGPU&    GetInstanceGPU() { return m_InstanceGPU; }
    inline MaterialsManager::MaterialGPU&       GetMaterialGPU() { return m_InstanceGPU.GetMaterialGPU(); }
    inline GlobalGeometry::GeometryGPU&         GetGeometryGPU() { return m_GeometryGPU; }
    inline InstanceFlags                        GetInstanceFlags() const { return m_InstanceGPU.GetFlags(); }

    static DRE::U32                             LayerToBits(Layer layer) { return 1u << DRE::U32(layer); }
    void                                        AddLayerPipeline(Layer layer, VKW::Pipeline* pipeline);

    // will put material ptr into InstanceGPU + cache MaterialGPU in the object
    void                                        SetMaterialGPU(MaterialsManager::MaterialGPU const& materialGPU);
    void                                        SetInstanceFlags(InstanceFlags flags);

private:
    void                                        SetFlag(InstanceFlags flag, bool enable);

private:
    WORLD::SceneNode*                               m_SceneNode;

    DRE::U32                                                m_LayerBits;
    DRE::InplaceVector<VKW::Pipeline*, Layer::LAYER_COUNT>   m_Pipelines;

    VKW::AccelerationStructureResource*             m_BLASResource;

    InstanceDataManager::InstanceGPU                m_InstanceGPU;
    GlobalGeometry::GeometryGPU                     m_GeometryGPU;
};

}
