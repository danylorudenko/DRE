#pragma once

#include <vulkan\vulkan.h>

#include <limits>

#include <vk_interface\Format.hpp>
#include <vk_interface\memory\MemoryPage.hpp>

namespace VKW
{


struct BufferResource
{
    BufferResource(VkBuffer handle, std::uint32_t size, MemoryRegion const& memory, std::uint64_t gpuAddress);

    VkBuffer        handle_     = VK_NULL_HANDLE;
    std::uint32_t   size_       = 0;
    MemoryRegion    memory_;
    std::uint64_t   gpuAddress_ = 0;

    MemoryPage* GetMemoryPage() const;
};

struct SubbufferResource
{
    VkBuffer        handle_ = VK_NULL_HANDLE;
    std::uint32_t   offset_ = 0;
    std::uint32_t   size_   = 0;
};



struct ImageResource
{
    ImageResource(VkImage handle, Format format, std::uint32_t width, std::uint32_t height, MemoryRegion const& memory, VkImageCreateInfo const& createInfo);

    VkImage             handle_ = VK_NULL_HANDLE;
    Format              format_ = FORMAT_UNDEFINED;
    std::uint32_t       width_  = 0;
    std::uint32_t       height_ = 0;
    MemoryRegion        memory_;
    VkImageCreateInfo   createInfo_;

    MemoryPage* GetMemoryPage() const;
};

struct ImageResourceView
{
    ImageResourceView(VkImageView handle, VkImageViewCreateInfo const& createInfo, ImageResource* parentResource);

    VkImageView             handle_         = VK_NULL_HANDLE;
    VkImageViewCreateInfo   createInfo_;
    ImageResource*          parentResource_ = nullptr;

    inline Format           GetFormat() const        { return VK2Format(createInfo_.format); }
    inline VkImageViewType  GetType() const          { return createInfo_.viewType; };
    inline std::uint32_t    GetMipCount() const      { return createInfo_.subresourceRange.levelCount; }
    inline std::uint32_t    GetLayerCount() const    { return createInfo_.subresourceRange.layerCount; }
    
    inline MemoryRegion*    GetMemoryRegion() const  { return &parentResource_->memory_; }
    inline MemoryPage*      GetMemoryPage() const    { return parentResource_->GetMemoryPage(); }
    inline VkImage          GetImageHandle() const   { return parentResource_->handle_; }
    inline Format           GetImageFormat() const     { return parentResource_->format_; }
    inline std::uint32_t    GetImageWidth() const    { return parentResource_->width_; }
    inline std::uint32_t    GetImageHeight() const   { return parentResource_->height_; }
};

}