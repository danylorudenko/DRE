#include <gfx\GraphicsManager.hpp>

#include <foundation\system\Window.hpp>


#include <vk_wrapper\Device.hpp>
#include <vk_wrapper\pipeline\Pipeline.hpp>
#include <vk_wrapper\pipeline\ShaderModule.hpp>

#include <gfx\pass\ForwardOpaquePass.hpp>
#include <gfx\pass\ImGuiRenderPass.hpp>

#include <engine\io\IOManager.hpp>
#include <global_uniform.h>

namespace GFX
{

static constexpr std::uint32_t C_DEFAULT_STAGING_ARENA_SIZE     = 1024 * 1024 * 64;
static constexpr std::uint32_t C_DEFAULT_UNIFORM_ARENA_SIZE     = DRE_U16_MAX - 1;
static constexpr std::uint32_t C_DEFAULT_READBACK_ARENA_SIZE    = 1024 * 1024 * 64;
static constexpr std::uint32_t C_GLOBAL_UNIFORM_SIZE            = 256;

GraphicsManager* g_GraphicsManager = nullptr;

GraphicsManager::GraphicsManager(HINSTANCE hInstance, Window* window, bool debug)
    : m_MainWindow{ window }
    , m_Device{ hInstance, window->NativeHandle(), debug}
    , m_MainContext{ m_Device.table_.get(), m_Device.GetMainQueue() }
    , m_GraphicsFrame{ 0 }
    , m_UploadArena{ &m_Device, C_DEFAULT_STAGING_ARENA_SIZE }
    , m_UniformArena{ &m_Device, C_DEFAULT_UNIFORM_ARENA_SIZE }
    , m_ReadbackArena{ &m_Device, C_DEFAULT_UNIFORM_ARENA_SIZE }
    , m_TextureBank{ &m_MainContext, m_Device.GetResourcesController(), m_Device.GetDescriptorAllocator() }
    , m_PipelineDB{ &m_Device }
    , m_PersistentStorage{ &m_Device }
    , m_RenderGraph{ this }
    , m_DependencyManager{ &m_RenderGraph.ResourcesManager() }
{
    g_GraphicsManager = this;

    for (std::uint32_t i = 0; i < VKW::CONSTANTS::FRAMES_BUFFERING; i++)
    {
        m_GlobalUniforms[i] = m_Device.GetResourcesController()->CreateBuffer(C_GLOBAL_UNIFORM_SIZE, VKW::BufferUsage::UNIFORM);
    }
    m_Device.GetDescriptorAllocator()->AllocateDefaultDescriptors(VKW::CONSTANTS::FRAMES_BUFFERING, m_GlobalUniforms, m_PersistentStorage.GetStorage().GetResource());
}

void GraphicsManager::Initialize()
{
    CreateAllPasses();
}

void GraphicsManager::CreateAllPasses()
{
    m_RenderGraph.AddPass<ForwardOpaquePass>();
    m_RenderGraph.AddPass<ImGuiRenderPass>();
    m_RenderGraph.ParseGraph();
    m_RenderGraph.InitGraphResources();
}

void GraphicsManager::PrepareGlobalData(VKW::Context& context, std::uint64_t deltaTimeUS)
{
    std::uint8_t C_STD140_DATA_STRIDE = 16;
    
    VKW::BufferResource* buffer = m_GlobalUniforms[GetCurrentFrameID()];
    void* dst = buffer->memory_.GetRegionMappedPtr();

    GlobalUniform globalUniform{};
    globalUniform.viewportSize_deltaMS_0[0] = static_cast<float>(GetRenderingWidth());
    globalUniform.viewportSize_deltaMS_0[1] = static_cast<float>(GetRenderingHeight());
    globalUniform.viewportSize_deltaMS_0[2] = static_cast<float>(static_cast<double>(deltaTimeUS) / 1000.0);
    globalUniform.viewportSize_deltaMS_0[3] = 0.0f;

    std::memcpy(dst, &globalUniform, sizeof(globalUniform));

    buffer->memory_.FlushCaches(g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice());

    context.CmdResourceDependency(buffer,
        VKW::RESOURCE_ACCESS_HOST_WRITE, VKW::STAGE_HOST,
        VKW::RESOURCE_ACCESS_SHADER_UNIFORM, VKW::STAGE_VERTEX);
}

void GraphicsManager::RenderFrame(std::uint64_t frame, std::uint64_t deltaTimeUS)
{
    m_GraphicsFrame = frame;

    m_UniformArena.ResetAllocations(GetCurrentFrameID());
    m_UploadArena.ResetAllocations(GetCurrentFrameID());
    m_ReadbackArena.ResetAllocations(GetCurrentFrameID());

    VKW::Context& context = GetMainContext();
    PrepareGlobalData(context, deltaTimeUS);

    // globalData
    context.CmdBindGlobalDescriptorSets(*GetMainDevice()->GetDescriptorAllocator(), GetCurrentFrameID());

    // main graph
    StorageTexture& finalRT = m_RenderGraph.Render(context);

    // presentation
    m_DependencyManager.ResourceBarrier(context, finalRT.GetResource(), VKW::RESOURCE_ACCESS_TRANSFER_SRC, VKW::STAGE_TRANSFER);
    TransferToSwapchainAndPresent(finalRT);
}

void GraphicsManager::TransferToSwapchainAndPresent(StorageTexture& src)
{
    VKW::PresentationController* presentController = GetPresentationController();
    VKW::PresentationContext presentationContext = presentController->AcquireNewPresentContext();


    GetMainContext().CmdResourceDependency(presentController->GetSwapchainResource(presentationContext),
        VKW::RESOURCE_ACCESS_UNDEFINED, VKW::STAGE_PRESENT,
        VKW::RESOURCE_ACCESS_CLEAR,     VKW::STAGE_TRANSFER);

    GetMainContext().CmdCopyImageToImage(presentController->GetSwapchainResource(presentationContext), src.GetResource());

    GetMainContext().CmdResourceDependency(presentController->GetSwapchainResource(presentationContext),
        VKW::RESOURCE_ACCESS_CLEAR,   VKW::STAGE_TRANSFER,
        VKW::RESOURCE_ACCESS_PRESENT, VKW::STAGE_PRESENT);

    GetMainContext().FlushWaitSwapchain(presentationContext);
    GetMainContext().Present(presentationContext);
}

GraphicsManager::~GraphicsManager()
{
    GetMainContext().WaitIdle();
}

}

