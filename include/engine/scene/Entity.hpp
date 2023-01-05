#pragma once

#include <gfx\renderer\RenderableObject.hpp>

namespace Data
{
class Material;
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
private:
    GFX::RenderableObject*  m_RenderableObject;
    Data::Material*         m_Material;
};

}

