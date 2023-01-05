#pragma once

#include <memory\Memory.hpp>
#include <foundation\Container\InplaceVector.hpp>
#include <foundation\Container\Vector.hpp>

#include <scene\Camera.hpp>
#include <scene\Entity.hpp>

namespace WORLD
{

class Scene
{
public:
    Scene();

    inline Camera& GetMainCamera() { return m_MainCamera; }
    inline auto& GetEntities() { return m_SceneEntities; }

    Entity&     CreateEntity(Data::Material* material);

private:
    Camera m_MainCamera;

    DRE::Vector<Entity, DRE::MainAllocator> m_SceneEntities;
};

extern Scene* g_MainScene;

}

