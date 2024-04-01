#include <engine\scene\Entity.hpp>

#include <gfx\renderer\RenderableObject.hpp>
#include <gfx\GraphicsManager.hpp>

namespace WORLD
{

Entity::Entity()
    : ISceneNodeUser{ nullptr, ISceneNodeUser::Type::Entity }
    , m_RenderableObject{ nullptr }
    , m_Material{ nullptr }
    , m_Geometry{ nullptr }
{
}

Entity::~Entity()
{
}

}