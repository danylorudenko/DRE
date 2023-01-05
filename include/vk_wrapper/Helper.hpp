#pragma once

#include <cstdint>
#include <vulkan\vulkan.h>

#include <vk_interface\descriptor\Descriptor.hpp>

namespace VKW
{

class Swapchain;

namespace HELPER
{

VkRect2D                    SwapchainRect2D(Swapchain* swapchain);

VkDescriptorType            DescriptorTypeToVK(DescriptorType);
VkPipelineStageFlags        DescriptorStageToVK(DescriptorStage stages);

VkImageSubresource          DefaultImageSubresource();
VkImageSubresourceLayers    DefaultImageSubresourceLayers();
VkImageSubresourceRange     DefaultImageSubresourceRange(VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);

VkBufferMemoryBarrier       BarrierUploadToTransferSrc(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, std::uint32_t queueFamily);
VkBufferMemoryBarrier       BarrierTransferDstToCompute(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, std::uint32_t queueFamily);
VkBufferMemoryBarrier       BarrierTransferDstToShaderRead(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, std::uint32_t queueFamily);
VkBufferMemoryBarrier       BarrierTransferDstToHost(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, std::uint32_t queueFamily);
VkBufferMemoryBarrier       BarrierComputeToTransferSrc(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, std::uint32_t queueFamily);
VkImageMemoryBarrier        BarrierUndefinedToTransferDst(VkImage image, std::uint32_t queueFamily);
VkImageMemoryBarrier        BarrierTransferDstToTexture(VkImage image, std::uint32_t queueFamily);


}
}

// these operators should be in global scope

bool operator==(VkSubpassDependency2 const& lhs, VkSubpassDependency2 const& rhs);
bool operator==(VkMemoryBarrier2KHR const& lhs, VkMemoryBarrier2KHR const& rhs);

