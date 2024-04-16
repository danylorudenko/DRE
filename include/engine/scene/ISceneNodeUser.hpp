#pragma once

#include <glm\vec3.hpp>
#include <glm\mat4x4.hpp>
#include <glm\gtc\quaternion.hpp>

#include <engine\scene\SceneNode.hpp>

namespace WORLD
{

class ISceneNodeUser
{
public:
    enum class Type
    {
        Entity,
        Camera,
        Light,
        MAX
    };


    ISceneNodeUser(SceneNode* node, Type type)
        : m_SceneNode{ node }
        , m_UserType{ type }
    {
    }

    inline SceneNode*       GetSceneNode() const { return m_SceneNode; }
    inline void             SetSceneNode(SceneNode* node) { DRE_ASSERT(m_SceneNode == nullptr, "Can't set SceneNode if it was already set."); m_SceneNode = node; }

    inline Type             GetType() const { return m_UserType; }

    inline glm::vec3 const& GetPosition() const { return m_SceneNode->GetPosition(); }
    inline glm::quat const& GetOrientation() const { return m_SceneNode->GetOrientation(); }
    inline glm::vec3        GetEulerOrientation() const { return m_SceneNode->GetEulerOrientation(); }
    inline float            GetScale() const { return m_SceneNode->GetScale(); }

    inline glm::vec3 const& GetForward() const { return m_SceneNode->GetForward(); }
    inline glm::vec3 const& GetRight() const { return m_SceneNode->GetRight(); }
    inline glm::vec3 const& GetUp() const { return m_SceneNode->GetUp(); }

    inline glm::vec3        GetGlobalPosition() const { return m_SceneNode->GetGlobalPosition(); }
    inline glm::quat        GetGlobalOrientation() const { return m_SceneNode->GetGlobalOrientation(); }
    inline glm::vec3        GetGlobalEulerOrientation() const { return m_SceneNode->GetGlobalEulerOrientation(); }
    inline float            GetGlobalScale() const { return m_SceneNode->GetGlobalScale(); }

    inline glm::mat4        GetGlobalMatrix() const { return m_SceneNode->GetGlobalMatrix(); }
    inline glm::mat4        GetGlobalMatrixNoScale() const { return m_SceneNode->GetGlobalMatrixNoScale(); }

    inline void             SetMatrix(glm::mat4 const& matrix) { m_SceneNode->SetMatrix(matrix); }


    inline void             SetPosition(glm::vec3 const& position) { m_SceneNode->SetPosition(position); }
    inline void             SetOrientation(glm::quat const& orientation) { m_SceneNode->SetOrientation(orientation); }
    inline void             SetEulerOrientation(glm::vec3 const& orientation) { m_SceneNode->SetEulerOrientation(orientation); }
    inline void             SetScale(float scale) { m_SceneNode->SetScale(scale); }

    inline void             Move(glm::vec3 const& movement) { m_SceneNode->Move(movement); }
    inline void             Rotate(glm::quat const& rotation) { m_SceneNode->Rotate(rotation); }
    inline void             Rotate(glm::vec3 const& eulerRotation) { m_SceneNode->Rotate(eulerRotation); }

protected:
    SceneNode*  m_SceneNode;
    Type        m_UserType;
};

}

