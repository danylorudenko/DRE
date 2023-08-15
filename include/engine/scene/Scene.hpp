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

class Scene
{
public:
    using EntityID = std::uint32_t;
    using NodeID = std::uint32_t;

    Scene(DRE::DefaultAllocator* allocator);
    ~Scene();


    inline Camera&                  GetMainCamera() { return m_MainCamera; }
    inline Camera const &           GetMainCamera() const { return m_MainCamera; }

    inline DirectionalLight&        GetMainSunLight() { return m_MainSunLight; }
    inline DirectionalLight const&  GetMainSunLight() const { return m_MainSunLight; }

    inline Entity*                  GetEntity(EntityID id) { return m_SceneEntities.Find(id).value; }
    inline SceneNode*               GetNode(NodeID id) { return m_Nodes.Find(id).value; }


    Entity*                         CreateOpaqueEntity(VKW::Context& context, Data::Geometry* geometry, Data::Material* material, SceneNode* parent = nullptr);


private:
    inline SceneNode*               CreateSceneNode(SceneNode* parent = nullptr) { NodeID const id = m_NodeCounter++; return &m_Nodes.Emplace(id, parent); }
    inline Entity*                  CreateEntity(GFX::RenderableObject* renderable = nullptr) { EntityID const id = m_EntityCounter++; return &m_SceneEntities.Emplace(id, renderable); }

private:
    Camera              m_MainCamera;
    DirectionalLight    m_MainSunLight;

    EntityID                                    m_EntityCounter;
    DRE::InplaceHashTable<EntityID, Entity>     m_SceneEntities;

    NodeID                                      m_NodeCounter;
    DRE::InplaceHashTable<NodeID, SceneNode>    m_Nodes;

    SceneNode* m_RootNode;
};

extern Scene* g_MainScene;

}

