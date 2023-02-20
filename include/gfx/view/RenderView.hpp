#pragma once

#include <foundation\Common.hpp>

#include <foundation\memory\Memory.hpp>
#include <foundation\Container\Vector.hpp>

#include <glm\vec2.hpp>
#include <glm\vec3.hpp>

#include <glm\mat4x4.hpp>

#include <gfx\renderer\RenderableObject.hpp>

namespace GFX
{

class RenderableObject;

class RenderView
{
public:
    RenderView();
    RenderView(DRE::DefaultAllocator* allocator);

    void UpdateViewport(glm::uvec2 offset, glm::uvec2 size);
    void UpdatePlacement(glm::vec3 viewerPos, glm::vec3 viewDirection, glm::vec3 up);
    void UpdateProjection(float fov, float zNear, float zFar);
    void UpdateProjection(float left, float right, float bottom, float top, float zNear, float zFar);

    inline glm::uvec2 const&    GetOffset() const { return m_Offset; }
    inline glm::uvec2 const&    GetSize() const { return m_Size; }

    inline glm::mat4 const&     GetViewM() const { return m_V; }
    inline glm::mat4 const&     GetInvViewM() const { return m_iV; }

    inline glm::mat4 const&     GetProjectionM() const { return m_P; }
    inline glm::mat4 const&     GetInvProjectionM() const{ return m_iP; }

    inline glm::mat4 const&     GetViewProjectionM() const { return m_VP; }
    inline glm::mat4 const&     GetInvViewProjection() const { return m_iVP; }

    inline auto const&          GetObjects() const { return m_Objects; }

    void AddObject(RenderableObject* renderableObject);
    void AddObjects(std::uint32_t count, RenderableObject* objects);

private:
    glm::uvec2 m_Offset;
    glm::uvec2 m_Size;

    glm::mat4  m_V;
    glm::mat4  m_iV;

    glm::mat4  m_P;
    glm::mat4  m_iP;

    glm::mat4  m_VP;
    glm::mat4  m_iVP;

    DRE::DefaultAllocator* m_Allocator;

    DRE::Vector<RenderableObject*, DRE::DefaultAllocator> m_Objects;
};

}

