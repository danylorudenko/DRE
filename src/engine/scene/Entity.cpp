#include <engine\scene\Entity.hpp>

namespace WORLD
{

Entity::Entity(TransformData const& transform, GFX::RenderableObject* renderable)
    : m_Transform{ transform }
    , m_RenderableObject{ renderable }
    , m_Material{ nullptr }
    , m_Geometry{ nullptr }
{
    if (m_RenderableObject)
    {
        m_RenderableObject->Transform(transform.model);
    }
}

}