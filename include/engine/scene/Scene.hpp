#pragma once

#include <foundation\memory\Memory.hpp>
#include <foundation\Container\InplaceVector.hpp>
#include <foundation\Container\Vector.hpp>

#include <engine\scene\Camera.hpp>
#include <engine\scene\Entity.hpp>

namespace Data
{
class Material;
class Geometry;
}

namespace VKW
{
class Context;
}

namespace WORLD
{

class Scene
{
public:
    Scene(DRE::DefaultAllocator* allocator);
    ~Scene();


    inline Camera&          GetMainCamera() { return m_MainCamera; }
    inline Camera const &   GetMainCamera() const { return m_MainCamera; }

    inline auto&            GetEntities() { return m_SceneEntities; }


    Entity&     CreateRenderableEntity(VKW::Context& context, Entity::TransformData const& transform, Data::Geometry* geometry, Data::Material* material);

private:
    Camera m_MainCamera;

    DRE::Vector<Entity, DRE::DefaultAllocator> m_SceneEntities;
};

extern Scene* g_MainScene;

}

