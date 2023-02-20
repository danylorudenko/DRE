#include <gfx\view\RenderView.hpp>

#include <glm\ext\matrix_transform.hpp>
#include <glm\ext\matrix_clip_space.hpp>

#include <foundation\memory\AllocatorLinear.hpp>

namespace GFX
{

RenderView::RenderView()
    : m_Offset{ 0, 0 }
    , m_Size{ 0, 0 }
    , m_V   { glm::identity<glm::mat4>() }
    , m_iV  { glm::identity<glm::mat4>() }
    , m_P   { glm::identity<glm::mat4>() }
    , m_iP  { glm::identity<glm::mat4>() }
    , m_VP  { glm::identity<glm::mat4>() }
    , m_iVP { glm::identity<glm::mat4>() }
    , m_Allocator{ nullptr }
{}

RenderView::RenderView(DRE::DefaultAllocator* allocator)
    : m_Offset{ 0, 0 }
    , m_Size{ 0, 0 }
    , m_V{ glm::identity<glm::mat4>() }
    , m_iV{ glm::identity<glm::mat4>() }
    , m_P{ glm::identity<glm::mat4>() }
    , m_iP{ glm::identity<glm::mat4>() }
    , m_VP{ glm::identity<glm::mat4>() }
    , m_iVP{ glm::identity<glm::mat4>() }
    , m_Allocator{ allocator }
    , m_Objects{ allocator }
{
    m_Objects.Reserve(2048);
}

void RenderView::UpdateViewport(glm::uvec2 offset, glm::uvec2 size)
{
    m_Offset = offset;
    m_Size = size;
}

void RenderView::UpdatePlacement(glm::vec3 viewerPos, glm::vec3 viewDirection, glm::vec3 up)
{
    m_V = glm::lookAtRH(viewerPos, viewerPos + viewDirection, up);
    m_iV = glm::transpose(m_V);

    m_VP = m_P * m_V;
    m_iVP = m_iP * m_iV;
}

void RenderView::UpdateProjection(float fov, float zNear, float zFar)
{
    float const aspect = static_cast<float>(m_Size[0]) / static_cast<float>(m_Size[1]);
    m_P = glm::perspectiveRH_ZO(glm::radians(fov), aspect, zNear, zFar);
    m_P[1][1] *= -1.0f;

    m_iP = glm::inverse(m_P);

    m_VP = m_P * m_V;
    m_iVP = m_iP * m_iV;
}

void RenderView::UpdateProjection(float left, float right, float bottom, float top, float zNear, float zFar)
{
    m_P = glm::orthoRH_ZO(left, right, bottom, top, zNear, zFar);
    m_P[1][1] *= -1.0f;

    m_iP = glm::inverse(m_P);

    m_VP = m_P * m_V;
    m_iVP = m_iP * m_iV;
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
