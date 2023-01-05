#pragma once

#include <cstdint>

#include <class_features\NonCopyable.hpp>
#include <class_features\NonMovable.hpp>

#include <memory\AllocatorLinear.hpp>
#include <foundation\Container\Vector.hpp>

namespace VKW
{
struct  BufferResource;
class   Pipeline;
}

namespace GFX
{

class RenderableObject;

struct AtomDraw
{
    VKW::BufferResource*    vertexBuffer;
    std::uint32_t           vertexOffset;
    std::uint32_t           vertexCount;

    VKW::BufferResource*    indexBuffer;
    std::uint32_t           indexOffset;
    std::uint32_t           indexCount;

    VKW::Pipeline*          pipeline;
};

class DrawBatcher
    : public NonMovable
    , public NonCopyable
{
public:
    DrawBatcher(DRE::AllocatorLinear* allocator);

    void AddRenderable(RenderableObject* object);

    void Batch();

    inline auto const& GetOpaqueDraws() const { return m_OpaqueDraws; }

private:
    DRE::AllocatorLinear*   m_Allocator;

    DRE::Vector<RenderableObject*, DRE::AllocatorLinear> m_AllRenderables;

    DRE::Vector<AtomDraw, DRE::AllocatorLinear> m_OpaqueDraws;
};

}

