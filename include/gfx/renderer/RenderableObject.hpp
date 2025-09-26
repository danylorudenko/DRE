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
    enum LayerBits
    {
        LAYER_NONE              = 0,
        LAYER_OPAQUE_BIT        = 1 << 0,
        LAYER_WATER_BIT         = 1 << 1
    };

    using DescriptorSetVector = DRE::InplaceVector<VKW::DescriptorSet, VKW::CONSTANTS::FRAMES_BUFFERING>;

    RenderableObject(
        WORLD::SceneNode* sceneNode, InstanceDataManager::InstanceGPU const& instanceGPU, LayerBits layers, VKW::Pipeline* pipeline,
        GlobalGeometry::GeometryGPU const& goemtryGPU,
        VKW::AccelerationStructureResource* blasResource
    );

    inline WORLD::SceneNode*                    GetSceneNode() const { return m_SceneNode; }
    inline LayerBits                            GetLayer() const { return m_Layer; }
    inline VKW::Pipeline*                       GetPipeline() const{ return m_Pipeline; }
    inline VKW::AccelerationStructureResource*  GetBLASResource() const { return m_BLASResource; }
    inline InstanceDataManager::InstanceGPU&    GetInstanceGPU() { return m_InstanceGPU; }
    inline MaterialsManager::MaterialGPU&       GetMaterialGPU() { return m_InstanceGPU.GetMaterialGPU(); }
    inline GlobalGeometry::GeometryGPU&         GetGeometryGPU() { return m_GeometryGPU; }
    inline InstanceFlags                        GetInstanceFlags() const { return m_InstanceGPU.GetFlags(); }

    // will put material ptr into InstanceGPU + cache MaterialGPU in the object
    void                                        SetMaterialGPU(MaterialsManager::MaterialGPU const& materialGPU);
    void                                        SetInstanceFlags(InstanceFlags flags);

    //void                                        SetDiffuseTexture(Texture* texture);
    //void                                        SetNormalTexture(Texture* texture);
    //void                                        SetMetalnessTexture(Texture* texture);
    //void                                        SetRoughnessTexture(Texture* texture);

    //void                                        SetNormalTexture(bool enable);
    //void                                        SetNormalTextureInvertY(bool enable);
    //void                                        SetNormalTBN(bool enable);
    //void                                        SetMaterialTexturesDefault(bool enable);
    //void                                        SetMaterialTexturesGLTFSpheres(bool enable);

private:
    void                                        SetFlag(InstanceFlags flag, bool enable);

private:
    WORLD::SceneNode*                   m_SceneNode;
    LayerBits                           m_Layer;
    VKW::Pipeline*                      m_Pipeline;
    VKW::AccelerationStructureResource* m_BLASResource;

    InstanceDataManager::InstanceGPU    m_InstanceGPU;
    GlobalGeometry::GeometryGPU         m_GeometryGPU;
};

}
