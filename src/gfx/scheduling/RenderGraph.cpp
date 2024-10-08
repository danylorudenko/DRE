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

void RenderGraph::RegisterTexture(BasePass* pass, char const* id, VKW::Format format, std::uint32_t width, std::uint32_t height, VKW::ResourceAccess access, VKW::Stages stage, std::uint32_t binding)
{
    m_ResourcesManager.RegisterTexture(id, format, width, height, access);
    m_DescriptorManager.RegisterTexture(pass->GetID(), id, access, VKW::StageToDescriptorStage(stage), binding);
}

void RenderGraph::RegisterStandaloneTexture(char const* id, VKW::Format format, std::uint32_t width, std::uint32_t height, VKW::ResourceAccess access)
{
    m_ResourcesManager.RegisterTexture(id, format, width, height, access);
}

void RenderGraph::RegisterTextureSlot(BasePass* pass, VKW::ResourceAccess access, VKW::Stages stage, std::uint32_t binding)
{
    m_DescriptorManager.RegisterTexture(pass->GetID(), RESOURCE_ID(TextureID::ID_None), access, VKW::StageToDescriptorStage(stage), binding);
}

void RenderGraph::RegisterRenderTarget(BasePass* pass, char const* id, VKW::Format format, std::uint32_t width, std::uint32_t height, std::uint32_t)
{
    m_ResourcesManager.RegisterTexture(id, format, width, height, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT);
}

void RenderGraph::RegisterDepthStencilTarget(BasePass* pass, char const* id, VKW::Format format, std::uint32_t width, std::uint32_t height)
{
    m_ResourcesManager.RegisterTexture(id, format, width, height, VKW::RESOURCE_ACCESS_DEPTH_STENCIL_ATTACHMENT);
}

void RenderGraph::RegisterDepthOnlyTarget(BasePass* pass, char const* id, VKW::Format format, std::uint32_t width, std::uint32_t height)
{
    m_ResourcesManager.RegisterTexture(id, format, width, height, VKW::RESOURCE_ACCESS_DEPTH_ONLY_ATTACHMENT);
}

void RenderGraph::RegisterStorageBuffer(BasePass* pass, char const* id, std::uint32_t size, VKW::ResourceAccess access, VKW::Stages stage, std::uint32_t binding)
{
    m_ResourcesManager.RegisterBuffer(id, size, access);
    m_DescriptorManager.RegisterBuffer(pass->GetID(), id, access, VKW::StageToDescriptorStage(stage), binding);
}

void RenderGraph::RegisterUniformBuffer(BasePass* pass, VKW::Stages stage, std::uint32_t binding)
{
    m_DescriptorManager.RegisterUniformBuffer(pass->GetID(), VKW::StageToDescriptorStage(stage), binding);
}

void RenderGraph::RegisterPushConstant(BasePass* pass, std::uint32_t size, VKW::Stages stage)
{
    m_DescriptorManager.RegisterPushConstant(pass->GetID(), size, VKW::StageToDescriptorStage(stage));
}

Texture* RenderGraph::GetTexture(char const* id)
{
    return m_ResourcesManager.GetTexture(id);
}

StorageBuffer* RenderGraph::GetBuffer(char const* id)
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

std::uint32_t RenderGraph::GetPassSetBinding()
{
    return g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->GetGlobalSetLayoutsCount();
}

std::uint32_t RenderGraph::GetUserSetBinding(PassID pass)
{
    return g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->GetGlobalSetLayoutsCount() +
        (GetPassDescriptorSet(pass, g_GraphicsManager->GetCurrentFrameID()).IsValid() ? 1 : 0);
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
    m_ResourcesManager.InitResources();
    m_DescriptorManager.InitDescriptors();

    for (std::uint32_t i = 0, size = m_Passes.Size(); i < size; i++)
    {
        m_Passes[i]->Initialize(*this);
    }
}

void RenderGraph::UnloadGraphResources()
{
    m_DescriptorManager.DestroyDescriptors();
    m_ResourcesManager.DestroyResources();
}

VKW::DescriptorSet RenderGraph::GetPassDescriptorSet(PassID pass, FrameID frameID)
{
    return m_DescriptorManager.GetPassDescriptorSet(pass, frameID);
}

VKW::PipelineLayout* RenderGraph::GetPassPipelineLayout(PassID pass)
{
    return m_DescriptorManager.GetPassPipelineLayout(pass);
}

Texture& RenderGraph::Render(VKW::Context& context)
{
    for (std::uint32_t i = 0, size = m_Passes.Size(); i < size; i++)
    {
        m_Passes[i]->Render(*this, context);
    }

    return *m_ResourcesManager.GetTexture(RESOURCE_ID(TextureID::DisplayEncodedImage));
}

GraphResourcesManager& RenderGraph::GetResourcesManager()
{
    return m_ResourcesManager;
}

}

