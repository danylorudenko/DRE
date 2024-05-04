#define IMGUI_IMPL_VULKAN_HAS_DYNAMIC_RENDERING
#define IMGUI_IMPL_VULKAN_NO_PROTOTYPES
#define VK_NO_PROTOTYPES

#include <demo_app\ImGuiHelper.hpp>

#include <foundation\input\InputSystem.hpp>
#include <foundation\system\Window.hpp>
#include <foundation\Common.hpp>

#include <engine\io\IOManager.hpp>

#include <Windows.h>

#include <vk_wrapper\Instance.hpp>
#include <vk_wrapper\Device.hpp>
#include <vk_wrapper\Context.hpp>

#include <backends\imgui_impl_vulkan.h>
#include <backends\imgui_impl_win32.h>
#include <backends\imgui_impl_vulkan.cpp>
#include <backends\imgui_impl_win32.cpp>

#include <gfx\GraphicsManager.hpp>

static ImGuiHelper* g_ImGuiHelper = nullptr;

void RenderDREImGui()
{
    g_ImGuiHelper->DrawFrame(GFX::g_GraphicsManager->GetMainContext());
}

ImGuiHelper::ImGuiHelper()
    : m_TargetWindow{ nullptr }
    , m_InputSystem{ nullptr }
{
}

VKW::Surface* ImGuiHelper::CreateCustomSurface(void* hwnd)
{
    VKW::SurfaceDesc desc;
    desc.table_ = GFX::g_GraphicsManager->GetVulkanTable();
    desc.instance_ = GFX::g_GraphicsManager->GetInstance();
    desc.device_ = GFX::g_GraphicsManager->GetMainDevice()->GetLogicalDevice();

    desc.hInstance_ = GetModuleHandle(nullptr);
    desc.hwnd_ = (HWND)hwnd;

    return &m_ImGuiSurfaces.EmplaceBack(desc);
}

void ImGuiHelper::DestroyCustomSurface()
{

}

static int Platform_CreateVkSurface(ImGuiViewport* vp, ImU64 vk_inst, const void* vk_allocators, ImU64* out_vk_surface)
{
    ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData(); 
    ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)vp->PlatformUserData;
    IM_UNUSED(bd);

    VKW::Surface* surface = g_ImGuiHelper->CreateCustomSurface(vd->Hwnd);
    *reinterpret_cast<VkSurfaceKHR*>(out_vk_surface) = surface->Handle();
    return (int)0;
}

ImGuiHelper::ImGuiHelper(Window* window, InputSystem* input, VKW::Instance& instance, VKW::Swapchain& swapchain, VKW::Device& device, VKW::Context& context)
    : m_TargetWindow{ window }
    , m_InputSystem{ input }
{
    g_ImGuiHelper = this;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    io.DisplaySize = ImVec2(swapchain.GetWidth(), swapchain.GetHeight());

    ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
    platformIO.Platform_CreateVkSurface = Platform_CreateVkSurface;


    ImGui_ImplWin32_Init(window->NativeHandle());

    struct LoadFuncsUserData
    {
        VKW::ImportTable* table;
        VKW::Instance* instance;
        VKW::Device* device;
    } userData;

    userData.table = device.GetFuncTable();
    userData.instance = &instance;
    userData.device = &device;

    ImGui_ImplVulkan_LoadFunctions([](const char* function_name, void* userData) 
        {
            auto* UD = (LoadFuncsUserData*)userData;

            if (std::strcmp(function_name, "vkCmdBeginRenderingKHR") == 0)
                function_name = "vkCmdBeginRendering";

            if (std::strcmp(function_name, "vkCmdEndRenderingKHR") == 0)
                function_name = "vkCmdEndRendering";

            auto* func = UD->table->vkGetDeviceProcAddr(UD->device->GetLogicalDevice()->Handle(), function_name);

            if(func == nullptr)
                func = UD->table->vkGetInstanceProcAddr(UD->instance->Handle(), function_name);

            DRE_ASSERT(func != nullptr, "ImGui failed to obtain necessary function pointers!");

            return func;
        },
        &userData
    );


    ImGui_ImplVulkan_InitInfo info;
    DRE::MemZero(&info, sizeof(info));

    info.Instance = instance.Handle();
    info.PhysicalDevice = device.GetLogicalDevice()->PhysicalDeviceHandle();
    info.Device = device.GetLogicalDevice()->Handle();
    info.QueueFamily = context.GetParentQueue()->GetQueueFamily();
    info.Queue = context.GetParentQueue()->GetHardwareQueue();
    info.DescriptorPool = device.GetDescriptorManager()->GetStandaloneDescriptorPool();
    info.RenderPass = VK_NULL_HANDLE; // ignored for dynamic rendering
    info.MinImageCount = VKW::CONSTANTS::FRAMES_BUFFERING;
    info.ImageCount = swapchain.GetImageCount();
    info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    info.PipelineCache = VK_NULL_HANDLE;
    info.Subpass = 0;

    info.UseDynamicRendering = true;
    info.MinAllocationSize = 0;

    VkFormat colorFormat = VKW::Format2VK(VKW::FORMAT_B8G8R8A8_UNORM);
    VkPipelineRenderingCreateInfoKHR renderingInfo;
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    renderingInfo.pNext = nullptr;
    renderingInfo.viewMask = 0;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &colorFormat;
    renderingInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    renderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
    info.PipelineRenderingCreateInfo = renderingInfo;

    ImGui_ImplVulkan_Init(&info);
    ImGui_ImplVulkan_CreateFontsTexture();
}

ImGuiHelper::ImGuiHelper(ImGuiHelper&& rhs)
    : m_TargetWindow{ nullptr }
    , m_InputSystem{ nullptr }
{
    operator=(DRE_MOVE(rhs));
}

ImGuiHelper& ImGuiHelper::operator=(ImGuiHelper&& rhs)
{
    DRE_SWAP_MEMBER(m_TargetWindow);
    DRE_SWAP_MEMBER(m_InputSystem);
    return *this;
}

ImGuiHelper::~ImGuiHelper()
{
    ImGui_ImplVulkan_DestroyFontsTexture();
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplWin32_Shutdown();
}

void ImGuiHelper::BeginFrame()
{
    ImGui::NewFrame();

    ImGui_ImplWin32_NewFrame();
    ImGui_ImplVulkan_NewFrame();
}

void ImGuiHelper::DrawFrame(VKW::Context& context)
{
    
}

void ImGuiHelper::EndFrame()
{
    ImGui::EndFrame();
    ImGui::UpdatePlatformWindows();
}


