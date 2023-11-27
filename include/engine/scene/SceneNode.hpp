#pragma once

#include <glm\vec3.hpp>
#include <glm\mat4x4.hpp>
#include <glm\gtc\quaternion.hpp>

#include <foundation\memory\Memory.hpp>
#include <foundation\Container\InplaceVector.hpp>
#include <foundation\Container\Vector.hpp>

#include <foundation\string\InplaceString.hpp>

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

class ISceneNodeUser;

class SceneNode
{
public:
    SceneNode();
    SceneNode(SceneNode* parent, ISceneNodeUser* user);

    DRE::U32                AddChild(SceneNode* child);
    void                    RemoveChild(SceneNode* child);

    SceneNode*              GetChild(DRE::U32 i);
    DRE::U32                GetChildrenCount() const { return m_Children.Size(); };

    void                    SetName(char const* name) { m_Name = name; }
    char const*             GetName() const { return m_Name.GetData(); }

    inline glm::vec3 const& GetPosition() const { return m_Position; }
    inline glm::quat const& GetOrientation() const { return m_Orientation; }
    glm::vec3               GetEulerOrientation() const;
    inline float            GetScale() const { return m_Scale; }

    inline glm::vec3 const& GetForward() const { return m_Forward; }
    inline glm::vec3 const& GetRight() const { return m_Right; }
    inline glm::vec3 const& GetUp() const { return m_Up; }

    glm::vec3               GetGlobalPosition() const;
    glm::quat               GetGlobalOrientation() const;
    glm::vec3               GetGlobalEulerOrientation() const;
    float                   GetGlobalScale() const;

    glm::mat4               GetGlobalMatrix() const;
    glm::mat4               GetGlobalMatrixNoScale() const;

    void                    SetMatrix(glm::mat4 const& matrix);


    inline void             SetPosition(glm::vec3 const& position) { m_Position = position; }
    void                    SetOrientation(glm::quat const& orientation);
    void                    SetEulerOrientation(glm::vec3 const& orientation);
    inline void             SetScale(float scale) { m_Scale = scale; }

    inline void             Move(glm::vec3 const& movement) { m_Position += movement; }
    void                    Rotate(glm::quat const& rotation);
    void                    Rotate(glm::vec3 const& eulerRotation);


    inline SceneNode* GetParent() const { return m_Parent; }
    void SetParent(SceneNode* parent);

    inline ISceneNodeUser* GetNodeUser() const { return m_NodeUser; }

private:
    void CalculateDirectionVectors();

private:
    SceneNode*      m_Parent;

    ISceneNodeUser* m_NodeUser;

    glm::vec3       m_Position;

    glm::quat       m_Orientation;
    glm::vec3       m_Forward;
    glm::vec3       m_Right;
    glm::vec3       m_Up;

    float           m_Scale;

    DRE::String64   m_Name;

    DRE::Vector<SceneNode*, DRE::DefaultAllocator> m_Children;
};

}

