#include <vk_wrapper\resources\Resource.hpp>

namespace VKW
{

BufferResource::BufferResource(VkBuffer handle, DRE::U32 size, MemoryRegion const& memory, DRE::U64 gpuAddress, char const* name)
    : handle_{ handle }
    , size_{ size }
    , memory_{ memory }
    , gpuAddress_{ gpuAddress }
    , name_{ name }
{

}

MemoryPage* BufferResource::GetMemoryPage() const
{
    return memory_.page_;
}

ImageResource::ImageResource(VkImage handle, Format format, DRE::U32 width, DRE::U32 height, DRE::U32 mipCount, MemoryRegion const& memory, VkImageCreateInfo const& createInfo, char const* name)
    : handle_{ handle }
    , format_{ format }
    , width_{ width }
    , height_{ height }
    , mipLevels_{ mipCount }
    , memory_{ memory }
    , createInfo_{ createInfo }
    , name_{ name }
{

}

MemoryPage* ImageResource::GetMemoryPage() const
{
    return memory_.page_;
}

ImageResourceView::ImageResourceView(VkImageView handle, VkImageViewCreateInfo const& createInfo, ImageResource* parentResource)
    : handle_{ handle }
    , createInfo_{ createInfo }
    , parentResource_{ parentResource }
{
}


}