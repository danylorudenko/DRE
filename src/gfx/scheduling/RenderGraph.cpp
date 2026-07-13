#include <gfx\scheduling\RenderGraph.hpp>

#include <foundation\Common.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\pass\BasePass.hpp>

//#define DRE_FLUSH_EVERY_PASS

namespace GFX
{

RenderGraph::RenderGraph(GraphicsManager* graphicsManager)
    : m_GraphicsManager{ graphicsManager }
    , m_ResourcesManager{ m_GraphicsManager->GetMainDevice() }
    , m_Passes{}
{
}

RenderGraph::~RenderGraph()
{
    for (std::uint32_t i = 0, size = m_Passes.Size(); i < size; i++)
    {
        delete m_Passes[i];
    }
}

void RenderGraph::RegisterTexture(BasePass* pass, char const* id, VKW::Format format, DRE::U32 width, DRE::U32 height, VKW::ResourceAccess access, GraphResourceFlags flags)
{
    m_ResourcesManager.RegisterTexture(id, format, width, height, access, flags);
}

void RenderGraph::RegisterRenderTarget(BasePass* pass, char const* id, VKW::Format format, DRE::U32 width, DRE::U32 height, DRE::U32 binding, GraphResourceFlags flags)
{
    m_ResourcesManager.RegisterTexture(id, format, width, height, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, flags);
}

void RenderGraph::RegisterDepthStencilTarget(BasePass* pass, char const* id, VKW::Format format, DRE::U32 width, DRE::U32 height, GraphResourceFlags flags)
{
    m_ResourcesManager.RegisterTexture(id, format, width, height, VKW::RESOURCE_ACCESS_DEPTH_STENCIL_ATTACHMENT, flags);
}

void RenderGraph::RegisterDepthOnlyTarget(BasePass* pass, char const* id, VKW::Format format, DRE::U32 width, DRE::U32 height, GraphResourceFlags flags)
{
    m_ResourcesManager.RegisterTexture(id, format, width, height, VKW::RESOURCE_ACCESS_DEPTH_ONLY_ATTACHMENT, flags);
}

void RenderGraph::RegisterStorageBuffer(BasePass* pass, char const* id, DRE::U32 size, VKW::ResourceAccess access, GraphResourceFlags flags)
{
    m_ResourcesManager.RegisterBuffer(id, size, access, flags);
}

Texture* RenderGraph::GetTexture(char const* id)
{
    return m_ResourcesManager.GetTexture(id);
}

StorageBuffer* RenderGraph::GetBuffer(char const* id)
{
    return m_ResourcesManager.GetBuffer(id);
}

Texture* RenderGraph::GetTemporalTextureCurrent(char const* id)
{
    return m_ResourcesManager.GetTemporalTexture(id, m_GraphicsManager->GetCurrentFrameID());
}

Texture* RenderGraph::GetTemporalTextureHistory(char const* id)
{
    return m_ResourcesManager.GetTemporalTexture(id, m_GraphicsManager->GetPrevFrameID());
}

StorageBuffer* RenderGraph::GetTemporalBufferCurrent(char const* id)
{
    return m_ResourcesManager.GetTemporalBuffer(id, m_GraphicsManager->GetCurrentFrameID());
}

StorageBuffer* RenderGraph::GetTemporalBufferHistory(char const* id)
{
    return m_ResourcesManager.GetTemporalBuffer(id, m_GraphicsManager->GetPrevFrameID());
}

UniformProxy RenderGraph::AllocateUniform(PassID id, VKW::Context& context, DRE::U32 size)
{
    UniformArena::Allocation allocation = m_GraphicsManager->GetUniformArena().AllocateTransientRegion(m_GraphicsManager->GetCurrentFrameID(), size, 256);
    return UniformProxy{ &context, allocation };
}

void RenderGraph::ParseGraph()
{
    for (DRE::U32 i = 0, size = m_Passes.Size(); i < size; i++)
    {
        m_Passes[i]->RegisterResources(*this);
    }
}

void RenderGraph::InitGraphResources()
{
    m_ResourcesManager.InitResources();

    for (DRE::U32 i = 0, size = m_Passes.Size(); i < size; i++)
    {
        m_Passes[i]->Initialize(*this);
    }
}

void RenderGraph::UnloadGraphResources()
{
    m_ResourcesManager.DestroyResources();
}

Texture& RenderGraph::Render(VKW::Context& context)
{
    for (DRE::U32 i = 0, size = m_Passes.Size(); i < size; i++)
    {
        m_Passes[i]->Render(*this, context);

#ifdef DRE_FLUSH_EVERY_PASS
        context.FlushAll();
        context.WaitIdle();
        context.CmdBindGlobalDescriptorSets(*m_GraphicsManager->GetMainDevice()->GetDescriptorManager(), m_GraphicsManager->GetCurrentFrameID());
#endif // DRE_FLUSH_EVERY_PASS
    }

    return *m_ResourcesManager.GetTexture(RESOURCE_ID(TextureID::DisplayEncodedImage));
}

GraphResourcesManager& RenderGraph::GetResourcesManager()
{
    return m_ResourcesManager;
}

}

