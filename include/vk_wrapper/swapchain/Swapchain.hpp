#pragma once

#include <vulkan\vulkan.h>

#include <class_features\NonCopyable.hpp>

#include <foundation\Container\InplaceVector.hpp>

#include <vk_interface\Format.hpp>
#include <vk_interface\Constant.hpp>

namespace VKW
{

class ImportTable;
class LogicalDevice;
class Surface;

struct SwapchainDesc
{
    ImportTable* table_;
    LogicalDevice* device_;
    Surface* surface_;

    std::uint32_t imagesCount_;
};

class Swapchain
    : public NonCopyable
{
public:
    struct SwapchainImage
    {
        VkImage                 image_;
        VkImageView             imageView_;
        VkImageViewCreateInfo   imageViewCreateInfo_;
    };

public:
    Swapchain();
    Swapchain(SwapchainDesc const& desc);

    Swapchain(Swapchain&& rhs);
    Swapchain& operator=(Swapchain&& rhs);

    ~Swapchain();

    VkSwapchainKHR  Handle() const;
    Format          GetFormat() const;
    std::uint32_t   GetWidth() const;
    std::uint32_t   GetHeight() const;
    std::uint32_t   GetImageCount() const;
    SwapchainImage& GetImage(std::uint32_t index);

    inline VkSwapchainCreateInfoKHR const& GetCreateInfo() const { return createInfo_; };
    inline VkImageCreateInfo const& GetImageCreateInfo() const { return createInfoImage_; }

private:
    VkImageCreateInfo SwapchainInfoToImageInfo(VkSwapchainCreateInfoKHR const& info);


private:
    ImportTable*    table_;
    LogicalDevice*         device_;

    Surface*        surface_;

    VkSwapchainKHR      swapchain_;

    VkSurfaceFormatKHR  swapchainFormat_;
    std::uint32_t       width_;
    std::uint32_t       height_;
    std::uint32_t       swapchainImageCount_;

    VkSwapchainCreateInfoKHR    createInfo_;
    VkImageCreateInfo           createInfoImage_;

    DRE::InplaceVector<SwapchainImage, VKW::CONSTANTS::FRAMES_BUFFERING> swapchainImages_;

};

}