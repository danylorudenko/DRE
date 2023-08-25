#include <engine\scene\Entity.hpp>

#include <gfx\renderer\RenderableObject.hpp>

namespace WORLD
{

Entity::Entity(GFX::RenderableObject* renderable)
    : ISceneNodeUser{ nullptr }
    , m_RenderableObject{ renderable }
    , m_Material{ nullptr }
    , m_Geometry{ nullptr }
{
}

void Entity::SetPosition(glm::vec3 const& pos)
{
    m_SceneNode->SetPosition(pos); m_RenderableObject->Transform(m_SceneNode->GetGlobalMatrix());
}

void Entity::SetOrientation(glm::vec3 const& euler)
{
    m_SceneNode->SetEulerOrientation(euler);
    m_RenderableObject->Transform(m_SceneNode->GetGlobalMatrix());
}

void Entity::Move(glm::vec3 const& movement)
{
    m_SceneNode->Move(movement);
    m_RenderableObject->Transform(m_SceneNode->GetGlobalMatrix());
}

void Entity::Rotate(glm::vec3 const& euler)
{
    m_SceneNode->Rotate(euler);
    m_RenderableObject->Transform(m_SceneNode->GetGlobalMatrix());
}

void Entity::SetMatrix(glm::mat4 m)
{
    m_SceneNode->SetMatrix(m);
    m_RenderableObject->Transform(m_SceneNode->GetGlobalMatrix());
}

}