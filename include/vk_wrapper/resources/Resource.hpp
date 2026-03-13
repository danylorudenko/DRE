#pragma once

#include <vulkan\vulkan.h>

#include <foundation\Common.hpp>
#include <foundation\string\InplaceString.hpp>

#include <vk_wrapper\Format.hpp>
#include <vk_wrapper\memory\MemoryPage.hpp>

namespace VKW
{


struct BufferResource
{
    BufferResource(VkBuffer handle, DRE::U32 size, MemoryRegion const& memory, DRE::U64 gpuAddress, char const* name);

    VkBuffer        handle_     = VK_NULL_HANDLE;
    DRE::U32        size_       = 0;
    MemoryRegion    memory_;
    DRE::U64        gpuAddress_ = 0;

    DRE::String128  name_;

    MemoryPage* GetMemoryPage() const;
};

struct SubbufferResource
{
    VkBuffer        handle_ = VK_NULL_HANDLE;
    DRE::U32        offset_ = 0;
    DRE::U32        size_   = 0;
};



struct ImageResource
{
    ImageResource(VkImage handle, Format format, DRE::U32 width, DRE::U32 height, DRE::U32 mipCount, MemoryRegion const& memory, VkImageCreateInfo const& createInfo, char const* name);

    VkImage             handle_ = VK_NULL_HANDLE;
    Format              format_ = FORMAT_UNDEFINED;
    DRE::U32            width_  = 0;
    DRE::U32            height_ = 0;
    DRE::U32            mipLevels_ = 0;
    MemoryRegion        memory_;
    VkImageCreateInfo   createInfo_;

    DRE::String128      name_;

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
    inline DRE::U32         GetMipCount() const      { return createInfo_.subresourceRange.levelCount; }
    inline DRE::U32         GetLayerCount() const    { return createInfo_.subresourceRange.layerCount; }
    
    inline MemoryRegion*    GetMemoryRegion() const  { return &parentResource_->memory_; }
    inline MemoryPage*      GetMemoryPage() const    { return parentResource_->GetMemoryPage(); }
    inline VkImage          GetImageHandle() const   { return parentResource_->handle_; }
    inline Format           GetImageFormat() const   { return parentResource_->format_; }
    inline DRE::U32         GetImageWidth() const    { return parentResource_->width_; }
    inline DRE::U32         GetImageHeight() const   { return parentResource_->height_; }
};



struct AccelerationStructureResource
{
    VkAccelerationStructureKHR      handle_             = VK_NULL_HANDLE;
    BufferResource*                 residenceBuffer_    = nullptr;
    VkDeviceAddress                 acAddress_          = 0;
};

}