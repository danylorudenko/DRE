#pragma once

#include <vulkan\vulkan.h>

#include <foundation\class_features\NonCopyable.hpp>

#include <vk_wrapper\ImportTable.hpp>
#include <vk_wrapper\Instance.hpp>

namespace VKW
{

enum class DeviceQueueType
{
    GRAPHICS_PRESENT,
    COMPUTE,
    TRANSFER
};

struct DeviceQueueFamilyInfo
{
    DeviceQueueType type_;
    std::uint32_t familyIndex_;
    std::uint32_t count_;
    bool presentationSupported_;
};

struct DeviceDesc
{
    ImportTable* table_; 
    Instance* instance_;
    std::vector<std::string> requiredExtensions_;

    std::uint32_t graphicsPresentQueueCount_;
    std::uint32_t computeQueueCount_;
    std::uint32_t transferQueueCount_;

};

class LogicalDevice
    : public NonCopyable
{
public:
    static std::uint32_t constexpr VENDOR_ID_NVIDIA = 0x10DE;
    static std::uint32_t constexpr VENDOR_ID_AMD = 0x1002;
    static std::uint32_t constexpr VENDOR_ID_INTEL = 0x8086;
    static std::uint32_t constexpr VENDOR_ID_ARM = 0x13B5;

public:
    struct PhysicalDeviceProperties
    {
        VkPhysicalDeviceProperties2 properties2;
        VkPhysicalDeviceMemoryProperties2 memoryProperties2;
        VkPhysicalDeviceMemoryBudgetPropertiesEXT memoryBudgetProperties;
        VkPhysicalDeviceVulkan12Properties vulkan12Properties;
        VkPhysicalDeviceDescriptorIndexingProperties descriptorIndexingProperties;
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;
        std::vector<VkExtensionProperties> extensionProperties;
        std::vector<std::uint32_t> presentationFamilies;
        VkPhysicalDeviceExtendedDynamicState3FeaturesEXT extendedDynamicState3FeaturesEXT;
        VkPhysicalDeviceFeatures2 features2;
        VkPhysicalDeviceVulkan12Features vulkan12Features;
        VkPhysicalDeviceVulkan13Features vulkan13Features;
    };

public:
    LogicalDevice();
    LogicalDevice(DeviceDesc const& rhs);

    LogicalDevice(LogicalDevice&& rhs);
    LogicalDevice& operator=(LogicalDevice&& rhs);

    ~LogicalDevice();

    VKW::LogicalDevice::PhysicalDeviceProperties const& Properties() const;
    
    std::uint32_t QueueFamilyCount() const;
    VKW::DeviceQueueFamilyInfo const& GetQueueFamily(std::uint32_t index) const;

    VkDevice Handle() const;
    VkPhysicalDevice PhysicalDeviceHandle() const;
    operator bool() const;

    bool IsAPI13Supported() const;
    void PrintPhysicalDeviceFormatProperties(VkFormat format);

private:
    static bool IsAPI13SupportedByPhysicalDevice(VkPhysicalDeviceProperties const& physicalDeviceProperties);

    static void PrintPhysicalDeviceData(
        VKW::LogicalDevice::PhysicalDeviceProperties const& deviceProperties);

    bool IsPhysicalDeviceValid(
        VKW::LogicalDevice::PhysicalDeviceProperties const& deviceProperties,
        std::vector<std::string> const& requiredExtensions
    );

    void DisableHeavyPhysicalDeviceFeatures();

    void RequestDeviceProperties(
        VkPhysicalDevice targetDevice,
        VKW::LogicalDevice::PhysicalDeviceProperties& deviceProperties
    );

private:
    VkDevice device_;
    ImportTable* table_;

    VkPhysicalDevice physicalDevice_;
    VKW::LogicalDevice::PhysicalDeviceProperties physicalDeviceProperties_;

    std::vector<DeviceQueueFamilyInfo> queueInfo_;
};

}