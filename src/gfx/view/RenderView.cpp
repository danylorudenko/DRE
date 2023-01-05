#include <gfx\view\RenderView.hpp>

#include <glm\ext\matrix_transform.hpp>
#include <glm\ext\matrix_clip_space.hpp>

#include <memory\AllocatorLinear.hpp>

namespace GFX
{

RenderView::RenderView()
    : m_Offset{ 0, 0 }
    , m_Dimentions{ 0, 0 }
    , m_V   { glm::identity<glm::mat4>() }
    , m_iV  { glm::identity<glm::mat4>() }
    , m_P   { glm::identity<glm::mat4>() }
    , m_iP  { glm::identity<glm::mat4>() }
    , m_VP  { glm::identity<glm::mat4>() }
    , m_iVP { glm::identity<glm::mat4>() }
    , m_Allocator{ nullptr }
{}

RenderView::RenderView(DRE::AllocatorLinear* allocator, glm::uvec2 offset, glm::uvec2 dimentions, glm::vec3 viewerPos, glm::vec3 viewDirection, glm::vec3 up, float fov)
    : m_Offset{ offset }
    , m_Dimentions{ dimentions }
    , m_V   { glm::lookAtLH(viewerPos, viewDirection, up) }
    , m_iV  { glm::inverse(m_V) }
    , m_P   { glm::perspectiveFovLH(fov, static_cast<float>(dimentions[0]), static_cast<float>(dimentions[1]), 0.0f, 1.0f ) }
    , m_iP  { glm::inverse(m_P) }
    , m_VP  { m_P * m_V }
    , m_iVP { glm::inverse(m_VP) }
    , m_Allocator{ allocator }
    , m_Objects{ allocator }
{
    m_Objects.Reserve(2048);
}

void RenderView::AddObjects(std::uint32_t count, RenderableObject* objects)
{
    for (std::uint32_t i = 0; i < count; i++)
    {
        m_Objects.EmplaceBack(objects + i);
    }
}

void RenderView::AddObject(RenderableObject* object)
{
    m_Objects.EmplaceBack(object);
}

}
