#pragma once

#include <foundation\Common.hpp>
#include <vulkan\vulkan.h>

#include <vk_wrapper\descriptor\Descriptor.hpp>

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
VkImageSubresourceRange     ImageSubresourceRange(VkImageAspectFlags aspectFlags, DRE::U32 mipCount);

VkBufferMemoryBarrier       BarrierUploadToTransferSrc(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, DRE::U32 queueFamily);
VkBufferMemoryBarrier       BarrierTransferDstToCompute(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, DRE::U32 queueFamily);
VkBufferMemoryBarrier       BarrierTransferDstToShaderRead(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, DRE::U32 queueFamily);
VkBufferMemoryBarrier       BarrierTransferDstToHost(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, DRE::U32 queueFamily);
VkBufferMemoryBarrier       BarrierComputeToTransferSrc(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, DRE::U32 queueFamily);


}
}

// these operators should be in global scope

bool operator==(VkSubpassDependency2 const& lhs, VkSubpassDependency2 const& rhs);
bool operator==(VkMemoryBarrier2KHR const& lhs, VkMemoryBarrier2KHR const& rhs);

