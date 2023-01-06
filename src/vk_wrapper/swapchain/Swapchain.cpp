#include <vk_wrapper\swapchain\Swapchain.hpp>

#include <foundation\Common.hpp>

#include <vk_wrapper\ImportTable.hpp>
#include <vk_wrapper\LogicalDevice.hpp>
#include <vk_wrapper\Surface.hpp>
#include <vk_wrapper\Tools.hpp>
#include <vk_wrapper\Constant.hpp>

#include <algorithm>

namespace VKW
{

Swapchain::Swapchain()
    : table_{ nullptr }
    , device_{ nullptr }
    , surface_{ nullptr }
    , swapchain_{ VK_NULL_HANDLE }
    , swapchainFormat_{ VK_FORMAT_UNDEFINED, VK_COLORSPACE_SRGB_NONLINEAR_KHR }
    , width_{ 0 }
    , height_{ 0 }
    , swapchainImageCount_{ 0 }
    , createInfo_{}
    , createInfoImage_{}
{
}

Swapchain::Swapchain(SwapchainDesc const& desc)
    : table_{ desc.table_ }
    , device_{ desc.device_ }
    , surface_{ desc.surface_ }
    , swapchain_{ VK_NULL_HANDLE }
    , swapchainFormat_{ VK_FORMAT_UNDEFINED, VK_COLORSPACE_SRGB_NONLINEAR_KHR }
    , width_{ 0 }
    , height_{ 0 }
    , swapchainImageCount_{ 0 }
    , createInfo_{}
    , createInfoImage_{}
{
    VkDevice const device = device_->Handle();

    auto const& surfaceFormats = surface_->SurfaceFormats();
    assert(surfaceFormats.size() > 0 && "Surface supportes no formats!");
    assert(surface_->SupportedQueueFamilies().size() > 0 && "Surface doesn't support any queue families on the initialized device.");

    auto const& validSurfaceFormat = surfaceFormats[0];

    createInfo_.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo_.pNext = nullptr;
    createInfo_.surface = surface_->Handle();
    createInfo_.minImageCount = desc.imagesCount_;
    createInfo_.imageFormat = validSurfaceFormat.format;
    createInfo_.imageColorSpace = validSurfaceFormat.colorSpace;
    createInfo_.imageArrayLayers = 1;
    createInfo_.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo_.pQueueFamilyIndices = nullptr;
    // transfer usage to enable vkCmdClearImage out of VkRenderPass scope
    createInfo_.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT; 
    createInfo_.imageExtent = surface_->SurfaceCapabilities().currentExtent;
    createInfo_.clipped = VK_FALSE;
    createInfo_.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo_.flags = VK_FLAGS_NONE;
    createInfo_.presentMode = /*VK_PRESENT_MODE_FIFO_KHR*/VK_PRESENT_MODE_IMMEDIATE_KHR;
    createInfo_.preTransform = surface_->SurfaceCapabilities().currentTransform;
    createInfo_.oldSwapchain = VK_NULL_HANDLE;
    VK_ASSERT(table_->vkCreateSwapchainKHR(device, &createInfo_, nullptr, &swapchain_));

    createInfoImage_ = Swapchain::SwapchainInfoToImageInfo(createInfo_);

    swapchainFormat_ = validSurfaceFormat;
    width_ = surface_->SurfaceCapabilities().currentExtent.width;
    height_ = surface_->SurfaceCapabilities().currentExtent.height;


    DRE::InplaceVector<VkImage, VKW::CONSTANTS::FRAMES_BUFFERING> swapchainImages;
    std::uint32_t swapchainImagesCount = 0;

    VK_ASSERT(table_->vkGetSwapchainImagesKHR(device, swapchain_, &swapchainImagesCount, nullptr));

    DRE_ASSERT(swapchainImagesCount <= CONSTANTS::FRAMES_BUFFERING, "Overflow of local image buffering.");

    swapchainImages.ResizeUnsafe(swapchainImagesCount);
    swapchainImageCount_ = swapchainImagesCount;
    VK_ASSERT(table_->vkGetSwapchainImagesKHR(device, swapchain_, &swapchainImagesCount, swapchainImages.Data()));

    for (std::uint32_t i = 0; i < swapchainImagesCount; ++i) {
        
        SwapchainImage swapchainImageWrapper;
        swapchainImageWrapper.image_ = swapchainImages[i];

        VkImageViewCreateInfo& viewInfo = swapchainImageWrapper.imageViewCreateInfo_;
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.pNext = nullptr;
        viewInfo.flags = VK_FLAGS_NONE;
        viewInfo.image = swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapchainFormat_.format;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VK_ASSERT(table_->vkCreateImageView(device, &viewInfo, nullptr, &swapchainImageWrapper.imageView_));
        
        swapchainImages_.EmplaceBack(swapchainImageWrapper);
    }
}

Swapchain::Swapchain(Swapchain&& rhs)
    : table_{ nullptr }
    , device_{ nullptr }
    , surface_{ nullptr }
    , swapchain_{ VK_NULL_HANDLE }
{
    operator=(std::move(rhs));
}

Swapchain& Swapchain::operator=(Swapchain&& rhs)
{
    std::swap(table_, rhs.table_);
    std::swap(device_, rhs.device_);
    std::swap(surface_, rhs.surface_);

    std::swap(swapchain_, rhs.swapchain_);
    std::swap(swapchainFormat_, rhs.swapchainFormat_);
    std::swap(width_, rhs.width_);
    std::swap(height_, rhs.height_);
    std::swap(swapchainImageCount_, rhs.swapchainImageCount_);
    std::swap(swapchainImages_, rhs.swapchainImages_);

    return *this;
}

Swapchain::~Swapchain()
{
    if (swapchain_!= VK_NULL_HANDLE) {
        for (std::uint8_t i = 0; i < swapchainImageCount_; i++)
        {
            table_->vkDestroyImageView(device_->Handle(), swapchainImages_[i].imageView_, nullptr);
        }

        table_->vkDestroySwapchainKHR(device_->Handle(), swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }
}

VkImageCreateInfo Swapchain::SwapchainInfoToImageInfo(VkSwapchainCreateInfoKHR const& info)
{
    VkImageCreateInfo result;
    result.sType        = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    result.pNext        = nullptr;
    result.flags        = VK_FLAGS_NONE;
    result.imageType    = VK_IMAGE_TYPE_2D;
    result.format       = info.imageFormat;
    result.extent.width = info.imageExtent.width;
    result.extent.height= info.imageExtent.height;
    result.mipLevels    = 1;
    result.arrayLayers  = 1;
    result.samples      = VK_SAMPLE_COUNT_1_BIT;
    result.tiling       = VK_IMAGE_TILING_MAX_ENUM;
    result.usage        = info.imageUsage;
    result.sharingMode  = info.imageSharingMode;
    result.queueFamilyIndexCount = 0;
    result.pQueueFamilyIndices = nullptr;
    result.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    return result;
}

VkSwapchainKHR Swapchain::Handle() const
{
    return swapchain_;
}

Format Swapchain::GetFormat() const
{
    return VK2Format(swapchainFormat_.format);
}

std::uint32_t Swapchain::GetWidth() const
{
    return width_;
}

std::uint32_t Swapchain::GetHeight() const
{
    return height_;
}

std::uint32_t Swapchain::GetImageCount() const
{
    return swapchainImageCount_;
}

Swapchain::SwapchainImage& Swapchain::GetImage(std::uint32_t index)
{
    return swapchainImages_[index];
}


}