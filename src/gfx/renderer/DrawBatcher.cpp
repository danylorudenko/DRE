#include <gfx\renderer\DrawBatcher.hpp>

namespace GFX
{

DrawBatcher::DrawBatcher(DRE::AllocatorLinear* allocator)
    : m_Allocator{ allocator }
    , m_AllRenderables{ allocator }
    , m_OpaqueDraws{ allocator }
{
}

void DrawBatcher::AddRenderable(RenderableObject* renderable)
{
    m_AllRenderables.EmplaceBack(renderable);
}

void DrawBatcher::Batch()
{
    /*struct AtomDraw
    {
        VKW::BufferResource*    vertexBuffer;
        std::uint32_t           vertexOffset;
        std::uint32_t           vertexCount;
    
        VKW::BufferResource*    indexBuffer;
        std::uint32_t           indexOffset;
        std::uint32_t           indexCount;
    
        VKW::Pipeline*          pipeline;
    };
    */

}


}