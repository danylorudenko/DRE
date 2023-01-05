#include <gfx\scheduling\RenderGraph.hpp>

#include <foundation\Common.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\pass\BasePass.hpp>
#include <gfx\scheduling\DependencyManager.hpp>

namespace GFX
{

RenderGraph::RenderGraph(GraphicsManager* graphicsManager)
    : m_GraphicsManager{ graphicsManager }
    , m_ResourcesManager{ m_GraphicsManager->GetMainDevice(), &m_GraphicsManager->GetUniformArena(), &m_GraphicsManager->GetReadbackArena() }
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
    m_ResourcesManager.RegisterTexture(id, format, width, height, access, stage);
    m_DescriptorManager.RegisterTexture(pass->GetID(), id, access, VKW::StageToDescriptorStage(stage), binding);
}

void RenderGraph::RegisterRenderTarget(BasePass* pass, TextureID id, VKW::Format format, std::uint32_t width, std::uint32_t height, std::uint32_t)
{
    m_ResourcesManager.RegisterTexture(id, format, width, height, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
}

void RenderGraph::RegisterDepthStencilTarget(BasePass* pass, TextureID id, VKW::Format format, std::uint32_t width, std::uint32_t height)
{
    m_ResourcesManager.RegisterTexture(id, format, width, height, VKW::RESOURCE_ACCESS_DEPTH_STENCIL_ATTACHMENT, VKW::STAGE_FRAGMENT);
}

void RenderGraph::RegisterStorageBuffer(BasePass* pass, BufferID id, std::uint32_t size, VKW::ResourceAccess access, VKW::Stages stage, std::uint32_t binding)
{
    m_ResourcesManager.RegisterBuffer(id, size, access, stage);
    m_DescriptorManager.RegisterBuffer(pass->GetID(), id, access, VKW::StageToDescriptorStage(stage), binding);
}

void RenderGraph::RegisterUniformBuffer(BasePass* pass, std::uint32_t size, VKW::Stages stage, std::uint32_t binding)
{
    UniformArena::Allocation& allocation = m_ResourcesManager.RegisterUniformBuffer(pass->GetID(), size, stage);
    m_DescriptorManager.RegisterUniformBuffer(pass->GetID(), allocation, VKW::StageToDescriptorStage(stage), binding);
}

void RenderGraph::RegisterPushConstant(BasePass* pass, std::uint32_t size, VKW::Stages stages)
{
    m_DescriptorManager.ResisterPushConstant(pass->GetID(), size, VKW::StageToDescriptorStage(stages));
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

VKW::StandaloneDescriptorSet& RenderGraph::GetPassDescriptorSet(PassID pass)
{
    return m_DescriptorManager.GetPassDescriptorSet(pass);
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

    return *m_ResourcesManager.GetStorageTexture(TextureID::FinalRT);
}

}

