#pragma once

#include <cstdint>
#include <vulkan\vulkan.h>

namespace VKW
{

class LogicalDevice;
class ImportTable;

enum class MemoryClass
{
    DeviceFast,
    CpuUniform,
    CpuStaging,
    CpuReadback,
    MAX
};

struct MemoryPage
{
    VkDeviceMemory          deviceMemory_   = VK_NULL_HANDLE;
    VkMemoryPropertyFlags   propertyFlags_  = 0;
    VkDeviceSize            size_           = 0;
    MemoryClass             memoryClass_    = MemoryClass::MAX;
    void*                   mappedMemoryPtr_ = nullptr;

    std::uint32_t           bindCount_      = 0;
    VkDeviceSize            nextFreeOffset_ = 0;

    VkDeviceSize GetFreeMemorySize() const { return size_ - nextFreeOffset_; }
};

struct MemoryRegion
{
    MemoryPage*     page_   = nullptr;
    std::uint64_t   offset_ = 0;
    std::uint64_t   size_   = 0;

    void FlushCaches(ImportTable* table, LogicalDevice* device) const;
    void* GetRegionMappedPtr() const;
};

}