#include "Resource.hpp"

namespace VKW
{

BufferResource::BufferResource(VkBuffer handle, std::uint32_t size, MemoryRegion const& memory, std::uint64_t gpuAddress)
    : handle_{ handle }, size_{ size }, memory_{ memory }, gpuAddress_{ gpuAddress }
{

}

MemoryPage* BufferResource::GetMemoryPage() const
{
    return memory_.page_;
}

ImageResource::ImageResource(VkImage handle, Format format, std::uint32_t width, std::uint32_t height, MemoryRegion const& memory, VkImageCreateInfo const& createInfo)
    : handle_{ handle }
    , format_{ format }
    , width_{ width }
    , height_{ height }
    , memory_{ memory }
    , createInfo_{ createInfo }
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