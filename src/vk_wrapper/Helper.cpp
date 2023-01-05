#include <vk_interface\Helper.hpp>

#include <foundation\Common.hpp>
#include <vk_interface\Tools.hpp>
#include <vk_interface\swapchain\Swapchain.hpp>

namespace VKW
{
namespace HELPER
{

VkRect2D SwapchainRect2D(Swapchain* swapchain)
{
    VkRect2D viewport{};
    viewport.offset.x = 0;
    viewport.offset.y = 0;
    viewport.extent.width = swapchain->GetWidth();
    viewport.extent.height = swapchain->GetHeight();

    return viewport;
}

VkDescriptorType DescriptorTypeToVK(DescriptorType type)
{
    switch (type)
    {
    case DescriptorTypeBits::DESCRIPTOR_TYPE_TEXTURE:
        return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

    case DescriptorTypeBits::DESCRIPTOR_TYPE_SAMPLER:
        return VK_DESCRIPTOR_TYPE_SAMPLER;

    case DescriptorTypeBits::DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    case DescriptorTypeBits::DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

    case DescriptorTypeBits::DESCRIPTOR_TYPE_STORAGE_IMAGE:
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

    case DescriptorTypeBits::DESCRIPTOR_TYPE_STORAGE_BUFFER:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    case DescriptorTypeBits::DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;

    default:
        assert(false && "Unsupported DescriptorType.");
        return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }
}

VkShaderStageFlags DescriptorStageToVK(DescriptorStage stage)
{
    VkShaderStageFlags stageFlags = VK_FLAGS_NONE;
    
    if (stage & DESCRIPTOR_STAGE_COMPUTE)
        stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;

    if (stage & DESCRIPTOR_STAGE_VERTEX)
        stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;

    if (stage & DESCRIPTOR_STAGE_FRAGMENT)
        stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;

    return stageFlags;
}

VkImageSubresource DefaultImageSubresource()
{
    return VkImageSubresource{
        VK_IMAGE_ASPECT_COLOR_BIT,
        0,
        0
    };
}

VkImageSubresourceLayers DefaultImageSubresourceLayers()
{
    return VkImageSubresourceLayers{
        VK_IMAGE_ASPECT_COLOR_BIT,
        0,
        0,
        1
    };
}

VkImageSubresourceRange DefaultImageSubresourceRange(VkImageAspectFlags aspectFlags)
{
    return VkImageSubresourceRange{
        aspectFlags,
        0,
        1,
        0,
        1
    };
}

VkImageMemoryBarrier BarrierUndefinedToTransferDst(VkImage image, std::uint32_t queueFamily)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.srcAccessMask = VK_FLAGS_NONE;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = queueFamily;
    barrier.dstQueueFamilyIndex = queueFamily;
    barrier.image = image;
    barrier.subresourceRange = DefaultImageSubresourceRange();

    return barrier;
}

VkImageMemoryBarrier BarrierTransferDstToTexture(VkImage image, std::uint32_t queueFamily)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcQueueFamilyIndex = queueFamily;
    barrier.dstQueueFamilyIndex = queueFamily;
    barrier.image = image;
    barrier.subresourceRange = DefaultImageSubresourceRange();

    return barrier;
}

VkBufferMemoryBarrier BarrierUploadToTransferSrc(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, std::uint32_t queueFamily)
{
    VkBufferMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.srcQueueFamilyIndex = queueFamily;
    barrier.dstQueueFamilyIndex = queueFamily;
    barrier.buffer = buffer;
    barrier.offset = offset;
    barrier.size = size;

    return barrier;
}

VkBufferMemoryBarrier BarrierTransferDstToCompute(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, std::uint32_t queueFamily)
{
    VkBufferMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    barrier.srcQueueFamilyIndex = queueFamily;
    barrier.dstQueueFamilyIndex = queueFamily;
    barrier.buffer = buffer;
    barrier.offset = offset;
    barrier.size = size;

    return barrier;
}

VkBufferMemoryBarrier BarrierTransferDstToShaderRead(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, std::uint32_t queueFamily)
{
    VkBufferMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.srcQueueFamilyIndex = queueFamily;
    barrier.dstQueueFamilyIndex = queueFamily;
    barrier.buffer = buffer;
    barrier.offset = offset;
    barrier.size = size;

    return barrier;
}

VkBufferMemoryBarrier BarrierTransferDstToHost(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, std::uint32_t queueFamily)
{
    VkBufferMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    barrier.srcQueueFamilyIndex = queueFamily;
    barrier.dstQueueFamilyIndex = queueFamily;
    barrier.buffer = buffer;
    barrier.offset = offset;
    barrier.size = size;

    return barrier;
}

VkBufferMemoryBarrier BarrierComputeToTransferSrc(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, std::uint32_t queueFamily)
{
    VkBufferMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.srcQueueFamilyIndex = queueFamily;
    barrier.dstQueueFamilyIndex = queueFamily;
    barrier.buffer = buffer;
    barrier.offset = offset;
    barrier.size = size;

    return barrier;
}

}
}

bool operator==(VkSubpassDependency2 const& lhs, VkSubpassDependency2 const& rhs)
{
    if (lhs.pNext != nullptr && rhs.pNext != nullptr)
    {
        VkMemoryBarrier2KHR const* blhs = reinterpret_cast<VkMemoryBarrier2KHR const*>(lhs.pNext);
        VkMemoryBarrier2KHR const* brhs = reinterpret_cast<VkMemoryBarrier2KHR const*>(rhs.pNext);

        return
            (lhs.srcSubpass == rhs.srcSubpass) &&
            (lhs.dstSubpass == rhs.dstSubpass) &&
            (blhs->srcStageMask     == brhs->srcStageMask) &&
            (blhs->dstStageMask     == brhs->dstStageMask) &&
            (blhs->srcAccessMask    == brhs->srcAccessMask) &&
            (blhs->dstAccessMask    == brhs->dstAccessMask);
    }
    else
    {
        return
            (lhs.srcSubpass == rhs.srcSubpass) &&
            (lhs.dstSubpass == rhs.dstSubpass) &&
            (lhs.srcStageMask == rhs.srcStageMask) &&
            (lhs.dstStageMask == rhs.dstStageMask) &&
            (lhs.srcAccessMask == rhs.srcAccessMask) &&
            (lhs.dstAccessMask == rhs.dstAccessMask) &&
            (lhs.dependencyFlags == rhs.dependencyFlags) &&
            (lhs.viewOffset == rhs.viewOffset);
    }
    
}

bool operator==(VkMemoryBarrier2KHR const& lhs, VkMemoryBarrier2KHR const& rhs)
{
    return
        (lhs.srcStageMask == rhs.srcStageMask) &&
        (lhs.srcAccessMask == rhs.srcAccessMask) &&
        (lhs.dstStageMask == rhs.dstStageMask) &&
        (lhs.dstAccessMask == rhs.dstAccessMask);
}

