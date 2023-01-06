#pragma once

#include <memory>

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\system\DynamicLibrary.hpp>

#include <vk_wrapper\ImportTable.hpp>
#include <vk_wrapper\Instance.hpp>
#include <vk_wrapper\LogicalDevice.hpp>
#include <vk_wrapper\Surface.hpp>
#include <vk_wrapper\swapchain\Swapchain.hpp>
#include <vk_wrapper\swapchain\PresentationController.hpp>
#include <vk_wrapper\memory\MemoryController.hpp>
#include <vk_wrapper\resources\ResourcesController.hpp>
#include <vk_wrapper\descriptor\DescriptorManager.hpp>
#include <vk_wrapper\queue\QueueProvider.hpp>

namespace IO
{
class IOManager;
}

namespace VKW
{

class Device final
    : public NonCopyable
{
public:
    Device(HINSTANCE hInstance,HWND hwnd, bool debug = false);

    Device(Device&& rhs) = default;
    Device& operator=(Device&& rhs) = default;

    ~Device();

    VKW::ImportTable*               GetFuncTable() const { return table_.get(); }
    VKW::LogicalDevice*             GetLogicalDevice() const{ return device_.get(); }
    VKW::Swapchain*                 GetSwapchain() const { return swapchain_.get(); }
    VKW::Queue*                     GetMainQueue() const { return queueProvider_->GetMainQueue(); }
    VKW::PresentationController*    GetPresentationController() const { return presentationController_.get(); }
    VKW::ResourcesController*       GetResourcesController() const { return resourcesController_.get(); }
    VKW::DescriptorManager*       GetDescriptorAllocator() const { return descriptorAllocator_.get(); }



    std::unique_ptr<DynamicLibrary> vulkanLibrary_;

    std::unique_ptr<VKW::ImportTable> table_;

    std::unique_ptr<VKW::Instance> instance_;
    std::unique_ptr<VKW::LogicalDevice> device_;
    std::unique_ptr<VKW::Surface> surface_;
    std::unique_ptr<VKW::Swapchain> swapchain_;

    std::unique_ptr<VKW::QueueProvider> queueProvider_;
    std::unique_ptr<VKW::PresentationController> presentationController_;

    std::unique_ptr<VKW::MemoryController> memoryController_;
    std::unique_ptr<VKW::ResourcesController> resourcesController_;

    std::unique_ptr<VKW::DescriptorManager> descriptorAllocator_;


};

}