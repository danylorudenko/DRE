#pragma once

#include <cstdint>

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>

#include <foundation\memory\AllocatorLinear.hpp>
#include <foundation\Container\Vector.hpp>

#include <vk_wrapper\descriptor\Descriptor.hpp>

#include <gfx\buffer\TransientArena.hpp>

namespace VKW
{
struct  BufferResource;
class   Pipeline;
class   DescriptorManager;
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
    VKW::DescriptorSet      descriptorSet;
};

class DrawBatcher
    : public NonMovable
    , public NonCopyable
{
public:
    DrawBatcher(DRE::AllocatorLinear* allocator, VKW::DescriptorManager* descriptorManager, UniformArena* uniformArena);

    void AddRenderable(RenderableObject* object);

    void Batch(VKW::Context& context);

    inline auto const& GetOpaqueDraws() const { return m_OpaqueDraws; }

private:
    DRE::AllocatorLinear*   m_Allocator;
    VKW::DescriptorManager* m_DescriptorManager;
    UniformArena*           m_UniformArena;

    DRE::Vector<RenderableObject*, DRE::AllocatorLinear> m_AllRenderables;
    DRE::Vector<AtomDraw, DRE::AllocatorLinear> m_OpaqueDraws;
};

}

