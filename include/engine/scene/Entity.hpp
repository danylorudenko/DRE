#pragma once

#include <gfx\renderer\RenderableObject.hpp>
#include <glm\mat4x4.hpp>
#include <glm\gtc\matrix_transform.hpp>

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
    struct TransformData
    {
        glm::mat4 model = glm::identity<glm::mat4>();
    };

public:
    Entity(TransformData const& transform, GFX::RenderableObject* renderable = nullptr);

    inline GFX::RenderableObject*   GetRenderableObject() { return m_RenderableObject; }
    inline void                     SetMaterial(Data::Material* material) { m_Material = material; }
    inline Data::Material*          GetMaterial() const { return m_Material; }
    inline void                     SetGeometry(Data::Geometry* geometry) { m_Geometry = geometry; }
    inline Data::Geometry*          GetGeometry() const { return m_Geometry; }

private:
    TransformData           m_Transform;
    GFX::RenderableObject*  m_RenderableObject;
    Data::Geometry*         m_Geometry;
    Data::Material*         m_Material;
};

}

