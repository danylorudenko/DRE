#include <gfx\scheduling\RenderGraph.hpp>

#include <foundation\Common.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\pass\BasePass.hpp>
#include <gfx\scheduling\DependencyManager.hpp>

namespace GFX
{

RenderGraph::RenderGraph(GraphicsManager* graphicsManager)
    : m_GraphicsManager{ graphicsManager }
    , m_ResourcesManager{ m_GraphicsManager->GetMainDevice() }
    , m_DescriptorManager{ m_GraphicsManager->GetMainDevice(), &m_ResourcesManager, &m_GraphicsManager->GetPipelineDB() }
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

void RenderGraph::RegisterStorageTexture(BasePass* pass, TextureID id, VKW::Format format, std::uint32_t width, std::uint32_t height, VKW::ResourceAccess access, VKW::Stages stage, std::uint32_t binding)
{
    m_ResourcesManager.RegisterTexture(id, format, width, height, access);
    m_DescriptorManager.RegisterTexture(pass->GetID(), id, access, VKW::StageToDescriptorStage(stage), binding);
}

void RenderGraph::RegisterRenderTarget(BasePass* pass, TextureID id, VKW::Format format, std::uint32_t width, std::uint32_t height, std::uint32_t)
{
    m_ResourcesManager.RegisterTexture(id, format, width, height, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT);
}

void RenderGraph::RegisterDepthStencilTarget(BasePass* pass, TextureID id, VKW::Format format, std::uint32_t width, std::uint32_t height)
{
    m_ResourcesManager.RegisterTexture(id, format, width, height, VKW::RESOURCE_ACCESS_DEPTH_STENCIL_ATTACHMENT);
}

void RenderGraph::RegisterDepthOnlyTarget(BasePass* pass, TextureID id, VKW::Format format, std::uint32_t width, std::uint32_t height)
{
    m_ResourcesManager.RegisterTexture(id, format, width, height, VKW::RESOURCE_ACCESS_DEPTH_ONLY_ATTACHMENT);
}

void RenderGraph::RegisterStorageBuffer(BasePass* pass, BufferID id, std::uint32_t size, VKW::ResourceAccess access, VKW::Stages stage, std::uint32_t binding)
{
    m_ResourcesManager.RegisterBuffer(id, size, access);
    m_DescriptorManager.RegisterBuffer(pass->GetID(), id, access, VKW::StageToDescriptorStage(stage), binding);
}

void RenderGraph::RegisterUniformBuffer(BasePass* pass, VKW::Stages stage, std::uint32_t binding)
{
    m_DescriptorManager.RegisterUniformBuffer(pass->GetID(), VKW::StageToDescriptorStage(stage), binding);
}

StorageTexture* RenderGraph::GetTexture(TextureID id)
{
    return m_ResourcesManager.GetTexture(id);
}

StorageBuffer* RenderGraph::GetBuffer(BufferID id)
{
    return m_ResourcesManager.GetBuffer(id);
}

UniformProxy RenderGraph::GetPassUniform(PassID id, VKW::Context& context, std::uint32_t size)
{
    UniformArena::Allocation allocation = m_GraphicsManager->GetUniformArena().AllocateTransientRegion(m_GraphicsManager->GetCurrentFrameID(), size, 256);

    VKW::DescriptorManager::WriteDesc writes;
    writes.AddUniform(allocation.m_Buffer, allocation.m_OffsetInBuffer, allocation.m_Size, m_DescriptorManager.GetPassUniformBinding(id));

    VKW::DescriptorSet passSet = GetPassDescriptorSet(id, m_GraphicsManager->GetCurrentFrameID());
    
    m_GraphicsManager->GetMainDevice()->GetDescriptorManager()->WriteDescriptorSet(passSet, writes);

    return UniformProxy{ &context, allocation };
}

void RenderGraph::ParseGraph()
{
    for (std::uint32_t i = 0, size = m_Passes.Size(); i < size; i++)
    {
        m_Passes[i]->RegisterResources(*this);
    }
}

void RenderGraph::InitGraphResources()
{
    m_ResourcesManager.PrepareResources();
    m_DescriptorManager.InitDescriptors();

    for (std::uint32_t i = 0, size = m_Passes.Size(); i < size; i++)
    {
        m_Passes[i]->Initialize(*this);
    }
}

VKW::DescriptorSet RenderGraph::GetPassDescriptorSet(PassID pass, FrameID frameID)
{
    return m_DescriptorManager.GetPassDescriptorSet(pass, frameID);
}

VKW::PipelineLayout* RenderGraph::GetPassPipelineLayout(PassID pass)
{
    return m_DescriptorManager.GetPassPipelineLayout(pass);
}

StorageTexture& RenderGraph::Render(VKW::Context& context)
{
    for (std::uint32_t i = 0, size = m_Passes.Size(); i < size; i++)
    {
        m_Passes[i]->Render(*this, context);
    }

    return *m_ResourcesManager.GetTexture(TextureID::DisplayEncodedImage);
}

}

