#pragma once

#include <glm\vec3.hpp>
#include <glm\mat4x4.hpp>

#include <foundation\memory\Memory.hpp>
#include <foundation\Container\InplaceVector.hpp>
#include <foundation\Container\Vector.hpp>

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

class SceneNode
{
public:
    SceneNode();
    SceneNode(SceneNode* parent);

    void AddChild(SceneNode* child);
    void RemoveChild(SceneNode* child);

    inline glm::vec3 const& GetPosition() const { return m_Position; }
    inline glm::vec3 const& GetEulerOrientation() const { return m_Orientation; }
    inline float            GetScale() const { return m_Scale; }

    inline glm::vec3 const& GetForward() const { return m_Forward; }
    inline glm::vec3 const& GetRight() const { return m_Right; }
    inline glm::vec3 const& GetUp() const { return m_Up; }

    glm::vec3               GetGlobalPosition() const;
    glm::vec3               GetGlobalOrientation() const;
    float                   GetGlobalScale() const;

    glm::mat4               GetGlobalMatrix() const;
    glm::mat4               GetGlobalMatrixNoScale() const;

    void                    SetMatrix(glm::mat4 const& matrix);


    inline void             SetPosition(glm::vec3 const& position) { m_Position = position; }
    inline void             SetEulerOrientation(glm::vec3 const& orientation) { m_Orientation = orientation; }
    inline void             SetScale(float scale) { m_Scale = scale; }

    inline void             Move(glm::vec3 const& movement) { m_Position += movement; }
    inline void             Rotate(glm::vec3 const& rotation) { m_Orientation += rotation; }


    inline SceneNode* GetParent() const { return m_Parent; }
    void SetParent(SceneNode* parent);

private:
    void CalculateDirectionVectors();

private:
    SceneNode*  m_Parent;

    glm::vec3   m_Position;

    glm::vec3   m_Orientation;
    glm::vec3   m_Forward;
    glm::vec3   m_Right;
    glm::vec3   m_Up;

    float       m_Scale;

    DRE::Vector<SceneNode*, DRE::DefaultAllocator> m_Children;
};

}

