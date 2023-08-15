#include <engine\scene\Entity.hpp>

namespace WORLD
{

Entity::Entity(GFX::RenderableObject* renderable)
    : m_RenderableObject{ renderable }
    , m_Material{ nullptr }
    , m_Geometry{ nullptr }
{
}

}