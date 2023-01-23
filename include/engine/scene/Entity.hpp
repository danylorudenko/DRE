#pragma once

#include <gfx\renderer\RenderableObject.hpp>

namespace Data
{
class Material;
class Geometry;
}

namespace WORLD
{

class Entity
{
public:
    Entity(glm::vec3 pos = {0,0,0}, glm::vec3 eulerRotation = {0,0,0}, glm::vec3 scale = {1,1,1}, GFX::RenderableObject* renderable = nullptr);

    inline GFX::RenderableObject*   GetRenderableObject() { return m_RenderableObject; }
    inline void                     SetMaterial(Data::Material* material) { m_Material = material; }
    inline Data::Material*          GetMaterial() const { return m_Material; }
    inline void                     SetGeometry(Data::Geometry* geometry) { m_Geometry = geometry; }
    inline Data::Geometry*          GetGeometry() const { return m_Geometry; }

private:
    GFX::RenderableObject*  m_RenderableObject;
    Data::Geometry*         m_Geometry;
    Data::Material*         m_Material;
};

}

