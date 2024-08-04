#include <gfx\view\RenderView.hpp>

#include <glm\ext\matrix_clip_space.hpp>

#include <foundation\memory\AllocatorLinear.hpp>

namespace GFX
{

RenderView::RenderView()
    : m_Allocator{ nullptr }
{}

RenderView::RenderView(DRE::DefaultAllocator* allocator)
    : m_Allocator{ allocator }
    , m_Objects{ allocator }
{
    m_Objects.Reserve(2048);
}

void RenderView::UpdateViewport(glm::uvec2 offset, glm::uvec2 size)
{
    m_Current.offset = offset;
    m_Current.size = size;
}

void RenderView::UpdatePlacement(glm::vec3 viewerPos, glm::vec3 viewDirection, glm::vec3 up)
{
    m_Current.pos = viewerPos;
    m_Current.dir = viewDirection;

    m_Current.V = glm::lookAtRH(viewerPos, viewerPos + viewDirection, up);
    m_Current.iV = glm::transpose(m_Current.V);

    m_Current.VP = m_Current.P * m_Current.V;
    m_Current.iVP = m_Current.iP * m_Current.iV;
}

void RenderView::UpdateProjection(float fov, float zNear, float zFar)
{
    float const aspect = static_cast<float>(m_Current.size[0]) / static_cast<float>(m_Current.size[1]);
    m_Current.P = glm::perspectiveRH_ZO(glm::radians(fov), aspect, zFar, zNear); // depth is reversed
    m_Current.P[1][1] *= -1.0f;

    m_Current.iP = glm::inverse(m_Current.P);

    m_Current.VP = m_Current.P * m_Current.V;
    m_Current.iVP = m_Current.iP * m_Current.iV;
}

void RenderView::UpdateProjection(float left, float right, float bottom, float top, float zNear, float zFar)
{
    m_Current.P = glm::orthoRH_ZO(left, right, bottom, top, zFar, zNear); // depth is reversed
    m_Current.P[1][1] *= -1.0f;

    m_Current.iP = glm::inverse(m_Current.P);

    m_Current.VP = m_Current.P * m_Current.V;
    m_Current.iVP = m_Current.iP * m_Current.iV;
}

void RenderView::UpdateJitter(float xJitter, float yJitter)
{
    m_Current.jitter.x = xJitter;
    m_Current.jitter.y = yJitter;
}

void RenderView::UpdatePreviosFrame()
{
    m_Prev = m_Current;
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
