#pragma once

#include <foundation\container\Vector.hpp>
#include <foundation\memory\Memory.hpp>

#include <glm\mat4x4.hpp>
#include <glm\gtc\matrix_transform.hpp>

#include <engine\scene\ISceneNodeUser.hpp>

namespace Data
{
class Material;
class Geometry;
}

namespace GFX
{
class RenderableObject;
};

namespace WORLD
{

class Entity : public ISceneNodeUser
{
public:
    Entity(GFX::RenderableObject* renderable = nullptr);

    inline GFX::RenderableObject*   GetRenderableObject() { return m_RenderableObject; }

    inline void                     SetMaterial(Data::Material* material) { m_Material = material; }
    inline Data::Material*          GetMaterial() const { return m_Material; }
    inline void                     SetGeometry(Data::Geometry* geometry) { m_Geometry = geometry; }
    inline Data::Geometry*          GetGeometry() const { return m_Geometry; }

    void                            SetPosition(glm::vec3 const& pos);
    void                            SetOrientation(glm::vec3 const& euler);

    void                            Move(glm::vec3 const& movement);
    void                            Rotate(glm::vec3 const& euler);

    void                            SetMatrix(glm::mat4 m);

    inline glm::vec3 const&         GetPosition() const { return m_SceneNode->GetPosition(); }
    inline glm::vec3 const&         GetOrientation() const { return m_SceneNode->GetEulerOrientation(); }

private:
    GFX::RenderableObject*  m_RenderableObject;
    Data::Geometry*         m_Geometry;
    Data::Material*         m_Material;
};

}

