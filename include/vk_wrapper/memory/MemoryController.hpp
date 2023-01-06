#pragma once

#include <cstdint>
#include <vulkan\vulkan.h>

#include <foundation\class_features\NonCopyable.hpp>

#include <foundation\Container\InplaceVector.hpp>

#include <vk_wrapper\Constant.hpp>
#include <vk_wrapper\memory\MemoryPage.hpp>

namespace VKW
{


struct MemoryPageRegionDesc
{
    std::uint64_t size_;
    std::uint64_t alignment_;
    std::uint32_t memoryTypeBits_;
    MemoryClass memoryClass_;
};



class LogicalDevice;
class ImportTable;

struct MemoryControllerDesc
{
    ImportTable* table_;
    LogicalDevice* device_;
};

class MemoryController
    : public NonCopyable
{
public:
    MemoryController();
    MemoryController(MemoryControllerDesc const& desc);

    MemoryController(MemoryController&& rhs);
    MemoryController& operator=(MemoryController&& rhs);

    ~MemoryController();

    MemoryRegion AllocateMemoryRegion(MemoryPageRegionDesc const& desc);
    void ReleaseMemoryRegion(MemoryRegion& region);

private:
    MemoryPage* AllocPage(MemoryClass memoryClass, std::uint64_t size);
    void FreePage(MemoryPage* page);

    MemoryRegion GetNextFreePageRegion(MemoryPage* page, MemoryPageRegionDesc const& desc);

    void AssignDefaultProperties();
    std::uint32_t FindBestMemoryType(std::uint32_t mandatoryFlags, std::uint32_t preferebleFlags, std::uint32_t nonPreferableFlags);

private:
    ImportTable*    table_;
    LogicalDevice*         device_;

    std::uint32_t   memoryClassTypes_[(int)MemoryClass::MAX];
    VkDeviceSize    defaultPageSizes_[(int)MemoryClass::MAX];

    DRE::InplaceVector<MemoryPage*, CONSTANTS::MAX_ALLOCATIONS> allocations_;
};

}