#include <scene\Entity.hpp>

namespace WORLD
{

Entity::Entity(glm::vec3 pos, glm::vec3 eulerRotation, glm::vec3 scale, GFX::RenderableObject* renderable)
    : m_RenderableObject{ renderable }
{
    if (m_RenderableObject)
    {
        m_RenderableObject->Transform(pos, eulerRotation, scale);
    }
}

}