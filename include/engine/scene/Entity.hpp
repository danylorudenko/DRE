#pragma once

#include <foundation\container\Vector.hpp>
#include <foundation\memory\Memory.hpp>

#include <glm\mat4x4.hpp>
#include <glm\gtc\matrix_transform.hpp>

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

class SceneNode;

class Entity
{
public:
    Entity(GFX::RenderableObject* renderable = nullptr);

    inline GFX::RenderableObject*   GetRenderableObject() { return m_RenderableObject; }

    inline void                     SetMaterial(Data::Material* material) { m_Material = material; }
    inline Data::Material*          GetMaterial() const { return m_Material; }
    inline void                     SetGeometry(Data::Geometry* geometry) { m_Geometry = geometry; }
    inline Data::Geometry*          GetGeometry() const { return m_Geometry; }
    
    inline void                     SetSceneNode(SceneNode* node) { m_SceneNode = node; }
    inline SceneNode*               GetSceneNode() const { return m_SceneNode; }

private:
    GFX::RenderableObject*  m_RenderableObject;
    Data::Geometry*         m_Geometry;
    Data::Material*         m_Material;

    SceneNode*              m_SceneNode;
};

}

