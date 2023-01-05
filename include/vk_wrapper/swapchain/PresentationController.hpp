#pragma once

#include <vulkan\vulkan.h>

#include <class_features\NonCopyable.hpp>

#include <foundation\Container\InplaceVector.hpp>
#include <foundation\Container\ObjectPoolQueue.hpp>

#include <vk_interface\Constant.hpp>
#include <vk_interface\resources\Resource.hpp>

namespace VKW
{

class ImportTable;
class LogicalDevice;
class Swapchain;
class QueueExecutionPoint;
class Queue;
class PresentationController;

struct PresentationSemaphoreWrapper
{
    PresentationSemaphoreWrapper(ImportTable* table, LogicalDevice* device, VkSemaphoreCreateInfo const& createInfo);
    ~PresentationSemaphoreWrapper();

    ImportTable* table_;
    LogicalDevice*      device_;
    VkSemaphore  semaphore_;
};

class PresentationContext
{
public:
    void               Present(Queue* context);

    VkSemaphore GetRenderingCompleteSemaphore() const;
    VkSemaphore GetSwapchainReleaseSemaphore() const;


private:
    friend class PresentationController;

    PresentationContext(
        PresentationController* controller, 
        PresentationSemaphoreWrapper* swapchainReleaseSemaphore, 
        PresentationSemaphoreWrapper* renderingCompleteSemaphore, 
        std::uint32_t imageId);


private:
    PresentationController* controller_;

    PresentationSemaphoreWrapper* swapchainReleaseSemaphore_;
    PresentationSemaphoreWrapper* renderingCompleteSemaphore_;

    std::uint32_t imageId_;
};

class PresentationController
    : public NonCopyable
{
public:
    PresentationController();
    PresentationController(ImportTable* table, LogicalDevice* device, Swapchain* swapchain, Queue* presentQueue);

    PresentationController(PresentationController&& rhs);
    PresentationController& operator=(PresentationController&& rhs);

    ~PresentationController();

    PresentationContext AcquireNewPresentContext();
    void Present(PresentationContext const& context, Queue* queue);

    inline Swapchain const*         GetSwapchain() const { return swapchain_; }
    inline ImageResource const*     GetSwapchainResource(PresentationContext const& context) const { return swapchainResources_.Data() + context.imageId_; }
    inline ImageResourceView const* GetSwapchainView(PresentationContext const& context) const { return swapchainResourceViews_.Data() + context.imageId_; }
    
private:
    ImportTable*    table_;
    LogicalDevice*         device_;
    Swapchain*      swapchain_;

    DRE::InplaceVector<ImageResource, CONSTANTS::FRAMES_BUFFERING>      swapchainResources_;
    DRE::InplaceVector<ImageResourceView, CONSTANTS::FRAMES_BUFFERING>  swapchainResourceViews_;

private:
    DRE::ObjectPoolQueue<PresentationSemaphoreWrapper> renderingCompleteSemaphoresPool_;
    DRE::ObjectPoolQueue<PresentationSemaphoreWrapper> swapchainReleaseSemaphoresPool_;
};

}