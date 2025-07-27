#pragma once

#include <cstdint>

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>

#include <foundation\memory\AllocatorLinear.hpp>
#include <foundation\Container\Vector.hpp>

#include <vk_wrapper\descriptor\Descriptor.hpp>

#include <gfx\buffer\TransientArena.hpp>
#include <gfx\renderer\RenderableObject.hpp>

namespace VKW
{
struct  BufferResource;
class   Pipeline;
class   DescriptorManager;
}

namespace WORLD
{
class Scene;
}

namespace GFX
{

class RenderableObject;
class RenderView;

struct AtomDraw
{
    VKW::BufferResource*    vertexBuffer;
    DRE::U32                vertexOffset;
    DRE::U32                vertexCount;

    VKW::BufferResource*    indexBuffer;
    DRE::U32                indexOffset;
    DRE::U32                indexCount;

    DRE::U32                instanceID;

    VKW::Pipeline*          pipeline;
    VKW::DescriptorSet      descriptorSet;
};

/////////////////////////////
class DrawBatcher
    : public NonMovable
    , public NonCopyable
{
public:
    DrawBatcher(DRE::AllocatorLinear* allocator, VKW::DescriptorManager* descriptorManager, UniformArena* uniformArena);

    using AtomDataDelegate = void(*)(RenderableObject& obj, VKW::Context& context, VKW::DescriptorManager& descriptorManager, UniformArena& arena, RenderView const& view, VKW::PipelineLayout const* layout);
    void Batch(VKW::Context& context, RenderView const& view, VKW::PipelineLayout const* layout, RenderableObject::LayerBits layers, AtomDataDelegate atomDelegate);
    void BatchShadow(VKW::Context& context, RenderView const& view, VKW::PipelineLayout const* layout, RenderableObject::LayerBits layers, AtomDataDelegate atomDelegate);

    inline auto const& GetDraws() const { return m_Draws; }

private:
    DRE::AllocatorLinear*   m_Allocator;
    VKW::DescriptorManager* m_DescriptorManager;
    UniformArena*           m_UniformArena;

    DRE::Vector<AtomDraw, DRE::AllocatorLinear> m_Draws;
};

}

