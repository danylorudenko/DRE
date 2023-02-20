#include <engine\scene\Entity.hpp>

namespace WORLD
{

Entity::Entity(TransformData const& transform, GFX::RenderableObject* renderable, GFX::RenderableObject* renderableShadow)
    : m_Transform{ transform }
    , m_RenderableObject{ renderable }
    , m_RenderableShadowObject{ renderableShadow }
    , m_Material{ nullptr }
    , m_Geometry{ nullptr }
{
    if (m_RenderableObject != nullptr)
    {
        m_RenderableObject->Transform(transform.model);
    }

    if (m_RenderableShadowObject != nullptr)
    {
        m_RenderableShadowObject->Transform(transform.model);
    }
}

}