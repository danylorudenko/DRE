#pragma once

#include <foundation\memory\Memory.hpp>
#include <foundation\Container\InplaceHashTable.hpp>
#include <foundation\Container\Vector.hpp>

#include <engine\scene\Camera.hpp>
#include <engine\scene\Entity.hpp>
#include <engine\scene\Light.hpp>
#include <engine\scene\SceneNode.hpp>

namespace Data
{
class Material;
class Geometry;
}

namespace VKW
{
class Context;
}

namespace GFX
{
class RenderableObject;
}



namespace WORLD
{

class ISceneNodeUser;

class Scene
{
public:
    using EntityID = std::uint32_t;
    using NodeID = std::uint32_t;
    using LightID = std::uint32_t;

    Scene(DRE::DefaultAllocator* allocator);
    ~Scene();

    SceneNode*                      GetRootNode() { return m_RootNode; }

    inline Camera&                  GetMainCamera() { return m_MainCamera; }
    inline Camera const &           GetMainCamera() const { return m_MainCamera; }

    inline Entity*                  GetEntity(EntityID id) { return m_SceneEntities.Find(id).value; }
    inline SceneNode*               GetNode(NodeID id) { return m_Nodes.Find(id).value; }

    inline Light*                   GetMainSunLight() { return m_MainLight; }
    inline void                     SetMainSunLight(Light* light) { m_MainLight = light; }


    Entity*                         CreateOpaqueEntity(VKW::Context& context, Data::Geometry* geometry, Data::Material* material, SceneNode* parent = nullptr);
    inline SceneNode*               CreateSceneNode(ISceneNodeUser* user, SceneNode* parent = nullptr);
    inline SceneNode*               CreateEmptySceneNode(SceneNode* parent = nullptr) { NodeID const id = m_NodeCounter++; return &m_Nodes.Emplace(id, parent, nullptr); }

    Light*                          CreateDirectionalLight(VKW::Context& context, SceneNode* parent = nullptr);

private:
    inline Entity*                  CreateEntity() { EntityID const id = m_EntityCounter++; return &m_SceneEntities.Emplace(id); }

private:
    Camera              m_MainCamera;
    Light*              m_MainLight;

    EntityID                                    m_EntityCounter;
    DRE::InplaceHashTable<EntityID, Entity>     m_SceneEntities;

    LightID                                     m_LightsCounter;
    DRE::InplaceHashTable<LightID, Light>       m_SceneLights;

    NodeID                                      m_NodeCounter;
    DRE::InplaceHashTable<NodeID, SceneNode>    m_Nodes;

    SceneNode* m_RootNode;
};

extern Scene* g_MainScene;

}

