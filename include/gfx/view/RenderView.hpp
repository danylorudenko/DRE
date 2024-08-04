#pragma once

#include <foundation\Common.hpp>

#include <foundation\memory\Memory.hpp>
#include <foundation\Container\Vector.hpp>

#include <glm\vec2.hpp>
#include <glm\vec3.hpp>

#include <glm\mat4x4.hpp>
#include <glm\ext\matrix_transform.hpp>

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
    void UpdateJitter(float xJitter, float yJitter);
    void UpdatePreviosFrame();

    inline glm::vec3 const&     GetPosition() const { return m_Current.pos; }
    inline glm::vec3 const&     GetDirection() const { return m_Current.dir; }

    inline glm::uvec2 const&    GetOffset() const { return m_Current.offset; }
    inline glm::uvec2 const&    GetSize() const { return m_Current.size; }

    inline glm::mat4 const&     GetViewM() const { return m_Current.V; }
    inline glm::mat4 const&     GetInvViewM() const { return m_Current.iV; }

    inline glm::mat4 const&     GetProjectionM() const { return m_Current.P; }
    inline glm::mat4 const&     GetInvProjectionM() const{ return m_Current.iP; }

    inline glm::mat4 const&     GetViewProjectionM() const { return m_Current.VP; }
    inline glm::mat4 const&     GetInvViewProjectionM() const { return m_Current.iVP; }


    inline glm::vec3 const&     GetPrevPosition() const { return m_Prev.pos; }
    inline glm::vec3 const&     GetPrevDirection() const { return m_Prev.dir; }

    inline glm::uvec2 const&    GetPrevOffset() const { return m_Prev.offset; }
    inline glm::uvec2 const&    GetPrevSize() const { return m_Prev.size; }

    inline glm::mat4 const&     GetPrevViewM() const { return m_Prev.V; }
    inline glm::mat4 const&     GetPrevInvViewM() const { return m_Prev.iV; }

    inline glm::mat4 const&     GetPrevProjectionM() const { return m_Prev.P; }
    inline glm::mat4 const&     GetPrevInvProjectionM() const{ return m_Prev.iP; }

    inline glm::mat4 const&     GetPrevViewProjectionM() const { return m_Prev.VP; }
    inline glm::mat4 const&     GetPrevInvViewProjectionM() const { return m_Prev.iVP; }


    inline auto const&          GetObjects() const { return m_Objects; }

    void AddObject(RenderableObject* renderableObject);
    void AddObjects(std::uint32_t count, RenderableObject* objects);

private:
    struct ViewParams
    {
        glm::vec3  pos        = glm::vec3{ 0.0f, 0.0f, 0.0f };
        glm::vec3  dir        = glm::vec3{ 0.0f, 0.0f, 0.0f };
        
        glm::uvec2 offset     = glm::uvec2{ 0, 0 };
        glm::uvec2 size       = glm::uvec2{ 0, 0 };
        glm::vec2  jitter     = glm::vec2{ 0.0f, 0.0f };

        glm::mat4  V          = glm::identity<glm::mat4>();
        glm::mat4  iV         = glm::identity<glm::mat4>();

        glm::mat4  P          = glm::identity<glm::mat4>();
        glm::mat4  iP         = glm::identity<glm::mat4>();

        glm::mat4  VP         = glm::identity<glm::mat4>();
        glm::mat4  iVP        = glm::identity<glm::mat4>();
    };

    ViewParams m_Current;
    ViewParams m_Prev;


    DRE::DefaultAllocator* m_Allocator;

    DRE::Vector<RenderableObject*, DRE::DefaultAllocator> m_Objects;
};

}

