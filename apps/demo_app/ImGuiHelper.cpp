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

ImGuiHelper* g_ImGuiHelper = nullptr;

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
    } userData;

    userData.table = device.GetFuncTable();
    userData.instance = &instance;

    ImGui_ImplVulkan_LoadFunctions([](const char* function_name, void* userData) 
        {
            auto* UD = (LoadFuncsUserData*)userData;
            auto* func = UD->table->vkGetInstanceProcAddr(UD->instance->Handle(), function_name);
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
    info.PipelineRenderingCreateInfo = *((VkPipelineRenderingCreateInfoKHR*)GFX::g_GraphicsManager->GetPipelineDB().GetPipeline("imgui")->GetDescriptor().CompileGraphicPipelineCreateInfo().pNext);
    info.MinAllocationSize = 0;

    ImGui_ImplVulkan_Init(&info);

    ImGui_ImplVulkan_CreateFontsTexture();

    /*
    IMGUI_IMPL_API void         ImGui_ImplVulkan_Shutdown();
    IMGUI_IMPL_API void         ImGui_ImplVulkan_NewFrame();
    IMGUI_IMPL_API void         ImGui_ImplVulkan_RenderDrawData(ImDrawData* draw_data, VkCommandBuffer command_buffer, VkPipeline pipeline = VK_NULL_HANDLE);
    IMGUI_IMPL_API bool         ImGui_ImplVulkan_CreateFontsTexture();
    IMGUI_IMPL_API void         ImGui_ImplVulkan_DestroyFontsTexture();
    */
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
    ImGui::Render();
    ImDrawData* data = ImGui::GetDrawData();
    VKW::Pipeline* pipeline = GFX::g_GraphicsManager->GetPipelineDB().GetPipeline("imgui");

    ImGui_ImplVulkan_RenderDrawData(data, *context.GetCurrentCommandList());

    //ImGui::RenderPlatformWindowsDefault();
}

void ImGuiHelper::EndFrame()
{
    ImGui::EndFrame();
    ImGui::UpdatePlatformWindows();
}


