#include <vk_wrapper\ImportTable.hpp>

#include <vk_wrapper\Constant.hpp>

#define VKW_IMPORT_VULKAN_FUNCTION(name) name = vulkanLibrary.GetProcAddress<PFN_##name##>(#name)
#define VKW_IMPORT_VULKAN_INSTANCE_FUNCTION(name) name = reinterpret_cast<PFN_##name##>(vkGetInstanceProcAddr(instance, #name))
#define VKW_IMPORT_VULKAN_DEVICE_FUNCTION(name) name = reinterpret_cast<PFN_##name##>(vkGetDeviceProcAddr(device, #name))

namespace VKW
{

ImportTable::ImportTable() = default;

ImportTable::ImportTable(DynamicLibrary& vulkanLibrary)
{
    assert(vulkanLibrary);

    VKW_IMPORT_VULKAN_FUNCTION(vkGetInstanceProcAddr);
    VKW_IMPORT_VULKAN_FUNCTION(vkGetDeviceProcAddr);

    VKW_IMPORT_VULKAN_FUNCTION(vkCreateInstance);
    VKW_IMPORT_VULKAN_FUNCTION(vkDestroyInstance);



    VKW_IMPORT_VULKAN_FUNCTION(vkEnumerateInstanceLayerProperties);
    VKW_IMPORT_VULKAN_FUNCTION(vkEnumerateInstanceExtensionProperties);
}

void ImportTable::GetInstanceProcAddresses(VkInstance instance)
{
    VKW_IMPORT_VULKAN_INSTANCE_FUNCTION(vkEnumeratePhysicalDevices);
    VKW_IMPORT_VULKAN_INSTANCE_FUNCTION(vkEnumerateDeviceExtensionProperties);

    VKW_IMPORT_VULKAN_INSTANCE_FUNCTION(vkGetPhysicalDeviceProperties);
    VKW_IMPORT_VULKAN_INSTANCE_FUNCTION(vkGetPhysicalDeviceProperties2);
    VKW_IMPORT_VULKAN_INSTANCE_FUNCTION(vkGetPhysicalDeviceFeatures);
    VKW_IMPORT_VULKAN_INSTANCE_FUNCTION(vkGetPhysicalDeviceFeatures2);
    VKW_IMPORT_VULKAN_INSTANCE_FUNCTION(vkGetPhysicalDeviceMemoryProperties);
    VKW_IMPORT_VULKAN_INSTANCE_FUNCTION(vkGetPhysicalDeviceMemoryProperties2);
    VKW_IMPORT_VULKAN_INSTANCE_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties);

#ifdef _WIN32
    VKW_IMPORT_VULKAN_INSTANCE_FUNCTION(vkGetPhysicalDeviceWin32PresentationSupportKHR);
    VKW_IMPORT_VULKAN_INSTANCE_FUNCTION(vkCreateWin32SurfaceKHR);
#endif
    VKW_IMPORT_VULKAN_INSTANCE_FUNCTION(vkDestroySurfaceKHR);
    VKW_IMPORT_VULKAN_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceSupportKHR);
    VKW_IMPORT_VULKAN_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    VKW_IMPORT_VULKAN_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceFormatsKHR);
    VKW_IMPORT_VULKAN_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfacePresentModesKHR);

    VKW_IMPORT_VULKAN_INSTANCE_FUNCTION(vkCreateDebugReportCallbackEXT);
    VKW_IMPORT_VULKAN_INSTANCE_FUNCTION(vkDestroyDebugReportCallbackEXT);

    VKW_IMPORT_VULKAN_INSTANCE_FUNCTION(vkCreateDevice);
    VKW_IMPORT_VULKAN_INSTANCE_FUNCTION(vkDeviceWaitIdle);
    VKW_IMPORT_VULKAN_INSTANCE_FUNCTION(vkDestroyDevice);
}

void ImportTable::GetDeviceProcAddresses(VkDevice device)
{
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCreateSwapchainKHR);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkGetSwapchainImagesKHR);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkAcquireNextImageKHR);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkDestroySwapchainKHR);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkQueuePresentKHR);

    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkAllocateMemory);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkFreeMemory);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkGetDeviceMemoryCommitment);

    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkMapMemory);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkUnmapMemory);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkFlushMappedMemoryRanges);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkInvalidateMappedMemoryRanges);

    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCreateBuffer);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkDestroyBuffer);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCreateImage);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkDestroyImage);

    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkGetPhysicalDeviceFormatProperties);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkGetPhysicalDeviceImageFormatProperties);

    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkGetBufferMemoryRequirements);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkGetImageMemoryRequirements);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkGetImageSubresourceLayout);

    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkBindBufferMemory);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkBindImageMemory);

    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCreateBufferView);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkDestroyBufferView);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCreateImageView);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkDestroyImageView);

    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkGetBufferDeviceAddress);

    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkGetDeviceQueue);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkQueueSubmit);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkQueueSubmit2KHR);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkQueueWaitIdle);
    
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCreateCommandPool);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkResetCommandPool);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkDestroyCommandPool);

    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkAllocateCommandBuffers);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkResetCommandBuffer);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkFreeCommandBuffers);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkBeginCommandBuffer);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkEndCommandBuffer);

    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCreateFence);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkResetFences);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkGetFenceStatus);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkWaitForFences);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkDestroyFence);

    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCreateSemaphore);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkDestroySemaphore);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkWaitSemaphores);

    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCreateShaderModule);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkDestroyShaderModule);

    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCreatePipelineCache);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkGetPipelineCacheData);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkMergePipelineCaches);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkDestroyPipelineCache);

    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCreateComputePipelines);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCreateGraphicsPipelines);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkDestroyPipeline);

    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCreateDescriptorSetLayout);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkDestroyDescriptorSetLayout);

    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCreatePipelineLayout);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkDestroyPipelineLayout);

    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCreateDescriptorPool);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkResetDescriptorPool);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkDestroyDescriptorPool);

    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkAllocateDescriptorSets);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkFreeDescriptorSets);

    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkUpdateDescriptorSets);

    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCreateSampler);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkDestroySampler);

    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCreateRenderPass);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkDestroyRenderPass);

    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCreateFramebuffer);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkDestroyFramebuffer);

    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCmdCopyBuffer2);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCmdCopyImage2);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCmdCopyBufferToImage2);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCmdCopyImageToBuffer2);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCmdBlitImage2);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCmdClearColorImage);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCmdClearDepthStencilImage);

    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCmdBindVertexBuffers);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCmdBindIndexBuffer);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCmdBindPipeline);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCmdBindDescriptorSets);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCmdPushConstants);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCmdSetViewport);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCmdSetScissor);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCmdDispatch);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCmdPipelineBarrier);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCmdPipelineBarrier2);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCmdBeginRenderPass);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCmdEndRenderPass);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCmdNextSubpass);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCmdBeginRendering);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCmdEndRendering);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCmdClearAttachments);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCmdDraw);
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCmdDrawIndexed);

#ifndef DRE_COMPILE_FOR_RENDERDOC
    VKW_IMPORT_VULKAN_DEVICE_FUNCTION(vkCmdSetPolygonModeEXT);
#endif // DRE_COMPILE_FOR_RENDERDOC
}

}