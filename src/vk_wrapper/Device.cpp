#include <vk_interface\Device.hpp>

#include <vk_interface/Constant.hpp>
#include <iostream>
#include <Windows.h>


namespace VKW
{

Device::Device(HINSTANCE hInstance, HWND hwnd, bool debug)
    : vulkanLibrary_{}
    , table_{}
{
    vulkanLibrary_ = std::make_unique<DynamicLibrary>("vulkan-1.dll");
    if (!vulkanLibrary_) {
        MessageBoxA(hwnd, "Failed to load vulkan-1.dll", NULL, MB_ICONERROR | MB_OK);
        return;
    }

    table_ = std::make_unique<ImportTable>(*vulkanLibrary_);

    auto instanceExtensions = std::vector<std::string>{ "VK_KHR_surface", VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
    if (debug)
        instanceExtensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

    auto instanceLayers = std::vector<std::string>{};
    if (debug) {
        instanceLayers.emplace_back("VK_LAYER_KHRONOS_validation");
    }



    VKW::InstanceDesc instanceDesc;
    instanceDesc.table_ = table_.get();
    instanceDesc.requiredInstanceExtensions_ = instanceExtensions;
    instanceDesc.requiredInstanceLayers_ = instanceLayers;
    instanceDesc.debug_ = debug;

    instance_ = std::make_unique<VKW::Instance>(instanceDesc);



    VKW::DeviceDesc deviceDesc;
    deviceDesc.table_ = table_.get();
    deviceDesc.instance_ = instance_.get();
    deviceDesc.requiredExtensions_ = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME };
    deviceDesc.graphicsPresentQueueCount_ = 1;
    deviceDesc.computeQueueCount_ = 0;
    deviceDesc.transferQueueCount_ = 0;

    device_ = std::make_unique<VKW::LogicalDevice>(deviceDesc);
    


    VKW::SurfaceDesc surfaceDesc;
    surfaceDesc.table_ = table_.get();
    surfaceDesc.instance_ = instance_.get();
    surfaceDesc.device_ = device_.get();
    surfaceDesc.hInstance_ = hInstance;
    surfaceDesc.hwnd_ = hwnd;

    surface_ = std::make_unique<VKW::Surface>(surfaceDesc);



    VKW::SwapchainDesc swapchainDesc;
    swapchainDesc.table_ = table_.get();
    swapchainDesc.device_ = device_.get();
    swapchainDesc.surface_ = surface_.get();
    swapchainDesc.imagesCount_ = 2;

    swapchain_ = std::make_unique<VKW::Swapchain>(swapchainDesc);

    queueProvider_ = std::make_unique<VKW::QueueProvider>(table_.get(), device_.get());
    presentationController_ = std::make_unique<VKW::PresentationController>(table_.get(), device_.get(), swapchain_.get(), queueProvider_->GetPresentationQueue());


    VKW::MemoryControllerDesc memoryControllerDesc;
    memoryControllerDesc.table_ = table_.get();
    memoryControllerDesc.device_ = device_.get();

    memoryController_ = std::make_unique<VKW::MemoryController>(memoryControllerDesc);



    resourcesController_ = std::make_unique<VKW::ResourcesController>(table_.get(), device_.get(), memoryController_.get());
    descriptorAllocator_ = std::make_unique<VKW::DescriptorManager>(table_.get(), device_.get());
}

Device::~Device()
{
    
}

}