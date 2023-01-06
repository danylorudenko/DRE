#include <vk_wrapper\memory\MemoryPage.hpp>

#include <vk_wrapper\ImportTable.hpp>
#include <vk_wrapper\LogicalDevice.hpp>
#include <vk_wrapper\Tools.hpp>

#include <foundation\memory\Pointer.hpp>

namespace VKW
{

void MemoryRegion::FlushCaches(ImportTable* table, LogicalDevice* device) const
{
    if (page_->propertyFlags_ & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        return;
    
    VkMappedMemoryRange range;
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.pNext = nullptr;
    range.memory = page_->deviceMemory_;
    range.offset = offset_;
    range.size = size_;

    VK_ASSERT(table->vkFlushMappedMemoryRanges(device->Handle(), 1, &range));
}

void* MemoryRegion::GetRegionMappedPtr() const
{
    return DRE::PtrAdd(page_->mappedMemoryPtr_, offset_);
}

}

