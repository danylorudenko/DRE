#include <vk_wrapper\swapchain\PresentationController.hpp>

#include <foundation\Common.hpp>

#include <vk_wrapper\Context.hpp>
#include <vk_wrapper\LogicalDevice.hpp>
#include <vk_wrapper\Helper.hpp>
#include <vk_wrapper\ImportTable.hpp>
#include <vk_wrapper\Tools.hpp>
#include <vk_wrapper\swapchain\Swapchain.hpp>
#include <vk_wrapper\queue\Queue.hpp>

namespace VKW
{

PresentationContext::PresentationContext(
    PresentationController* controller, 
    PresentationSemaphoreWrapper* swapchainReleaseSemaphore, 
    PresentationSemaphoreWrapper* renderingCompleteSemaphore, 
    std::uint32_t imageId)

    : controller_{ controller }
    , swapchainReleaseSemaphore_{ swapchainReleaseSemaphore }
    , renderingCompleteSemaphore_{ renderingCompleteSemaphore }
    , imageId_{ imageId }
{}

VkSemaphore PresentationContext::GetRenderingCompleteSemaphore() const 
{ 
    return renderingCompleteSemaphore_->semaphore_;
}
VkSemaphore PresentationContext::GetSwapchainReleaseSemaphore() const 
{
    return swapchainReleaseSemaphore_->semaphore_;
}

void PresentationContext::Present(Queue* queue)
{
    controller_->Present(*this, queue);
}



PresentationSemaphoreWrapper::PresentationSemaphoreWrapper(ImportTable* table, LogicalDevice* device, VkSemaphoreCreateInfo const& createInfo)
    : table_{ table }, device_{ device }, semaphore_{ VK_NULL_HANDLE }
{
    VK_ASSERT(table_->vkCreateSemaphore(device->Handle(), &createInfo, nullptr, &semaphore_));
}

PresentationSemaphoreWrapper::~PresentationSemaphoreWrapper()
{
    table_->vkDestroySemaphore(device_->Handle(), semaphore_, nullptr);
}

PresentationController::PresentationController()
    : table_{ nullptr }
    , device_{ nullptr }
    , swapchain_{ nullptr }
{

}

PresentationController::PresentationController(ImportTable* table, LogicalDevice* device, Swapchain* swapchain, Queue* presentQueue)
    : table_{ table }
    , device_{ device }
    , swapchain_{ swapchain }
{
    VkSemaphoreCreateInfo semaphoreInfo;
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreInfo.pNext = nullptr;
    semaphoreInfo.flags = VK_FLAGS_NONE;

    std::uint32_t const swapchainImagesCount = swapchain_->GetImageCount();
    DRE_ASSERT(swapchainImagesCount <= VKW::CONSTANTS::FRAMES_BUFFERING, "Swapchian has more frames than we can buffer in PresentationController.");

    renderingCompleteSemaphoresPool_.Init(swapchainImagesCount + 2, table_, device_, semaphoreInfo);
    swapchainReleaseSemaphoresPool_.Init(swapchainImagesCount + 2, table_, device_, semaphoreInfo);

    for (std::uint8_t i = 0; i < swapchainImagesCount; ++i) {
        swapchainResources_.EmplaceBack(VKW::ImageResource{ swapchain->GetImage(i).image_, swapchain->GetFormat(), swapchain->GetWidth(), swapchain->GetHeight(), VKW::MemoryRegion{}, swapchain->GetImageCreateInfo(), "SWAPCHAIN" });
        swapchainResourceViews_.EmplaceBack(VKW::ImageResourceView{ swapchain->GetImage(i).imageView_, swapchain->GetImage(i).imageViewCreateInfo_, &swapchainResources_.Last() });
    }
}

PresentationController::PresentationController(PresentationController&& rhs)
    : table_{ nullptr }
    , device_{ nullptr }
    , swapchain_{ nullptr }
{
    operator=(std::move(rhs));
}

PresentationController& PresentationController::operator=(PresentationController&& rhs)
{
    DRE_SWAP_MEMBER(table_);
    DRE_SWAP_MEMBER(device_);
    DRE_SWAP_MEMBER(swapchain_);

    DRE_SWAP_MEMBER(renderingCompleteSemaphoresPool_);
    DRE_SWAP_MEMBER(swapchainReleaseSemaphoresPool_);
    
    return *this;
}

PresentationController::~PresentationController()
{
}

PresentationContext PresentationController::AcquireNewPresentContext()
{
    std::uint32_t imageIndex = 0;

    PresentationSemaphoreWrapper* swapchainReleaseSemaphore = swapchainReleaseSemaphoresPool_.AcquireObject();

    VK_ASSERT(table_->vkAcquireNextImageKHR(
        device_->Handle(), 
        swapchain_->Handle(), 
        std::numeric_limits<std::uint64_t>::max() / 2, // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#vkAcquireNextImageKHR: timeout must not be UINT64_MAX
        swapchainReleaseSemaphore->semaphore_,
        VK_NULL_HANDLE,
        &imageIndex)
    );

    return PresentationContext{ this, swapchainReleaseSemaphore, renderingCompleteSemaphoresPool_.AcquireObject(), imageIndex};
}

void PresentationController::Present(PresentationContext const& context, Queue* queue)
{
    VkQueue presentQueue = queue->GetHardwareQueue();
    VkSwapchainKHR swapchain = swapchain_->Handle();

    VkResult results = VK_SUCCESS;

    VkPresentInfoKHR pInfo;
    pInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pInfo.pNext = nullptr;
    pInfo.waitSemaphoreCount = 1;
    pInfo.pWaitSemaphores = &context.renderingCompleteSemaphore_->semaphore_;
    pInfo.swapchainCount = 1;
    pInfo.pSwapchains = &swapchain;
    pInfo.pImageIndices = &context.imageId_;
    pInfo.pResults = &results;

    VK_ASSERT(table_->vkQueuePresentKHR(presentQueue, &pInfo));

    renderingCompleteSemaphoresPool_.ReturnObject(context.renderingCompleteSemaphore_);
    swapchainReleaseSemaphoresPool_.ReturnObject(context.swapchainReleaseSemaphore_);

}

}