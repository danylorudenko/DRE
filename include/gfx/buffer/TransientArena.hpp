#pragma once

#include <cstdint>

#include <foundation\class_features\NonCopyable.hpp>

#include <foundation\math\SimpleMath.hpp>
#include <foundation\memory\Pointer.hpp>
#include <foundation\Common.hpp>

#include <vk_wrapper\Device.hpp>
#include <vk_wrapper\resources\Resource.hpp>
#include <vk_wrapper\Constant.hpp>
#include <vk_wrapper\Context.hpp>
#include <vk_wrapper\memory\MemoryPage.hpp>
#include <vk_wrapper\Tools.hpp>

#include <gfx\DeviceChild.hpp>
#include <gfx\FrameID.hpp>

namespace GFX
{

template<VKW::BufferUsage USAGE>
class TransientArena
    : public DeviceChild
{
public:
    struct Allocation
    {
        VKW::BufferResource*    m_Buffer;
        std::uint32_t           m_OffsetInBuffer;
        void*                   m_MappedRange;
        std::uint64_t           m_GPUAddress;
        std::uint32_t           m_Size;

        FrameID                 m_FrameID;
        TransientArena*         m_Arena;

        void FlushCaches();
        void InvalidateRanges();
    };

public:
    TransientArena(VKW::Device* device, std::uint32_t size);

    TransientArena(TransientArena&& rhs);
    TransientArena& operator=(TransientArena&& rhs);

    virtual ~TransientArena();

    Allocation AllocateTransientRegion  (FrameID frameID, std::uint32_t size, std::uint32_t alignment);
    void FlushCaches                    (Allocation const& allocation);
    void InvalidateRanges               (Allocation const& allocation);

    void ResetAllocations               (FrameID contextID);

private:
    std::uint32_t m_TransientBuffersSize;
    std::uint8_t  m_TransientBuffersCount;

    struct AllocationContext
    {
        VKW::BufferResource*    m_Buffer = nullptr;
        void*                   m_MappedBufferBegin     = nullptr;
        void*                   m_CurrentBufferPtr      = nullptr;
    };
    static std::uint32_t ComputeFreeMemory(AllocationContext const& context);

    AllocationContext m_AllocationContexts[VKW::CONSTANTS::FRAMES_BUFFERING];

};


////////////////////////////////////////////
////////////////////////////////////////////
////////////////////////////////////////////


template<VKW::BufferUsage USAGE>
TransientArena<USAGE>::TransientArena(VKW::Device* device, std::uint32_t size)
    : DeviceChild(device)
    , m_TransientBuffersSize{ size }
    , m_TransientBuffersCount{ 0 }
{
    std::uint8_t constexpr buffering = VKW::CONSTANTS::FRAMES_BUFFERING;
    m_TransientBuffersCount = buffering;
    
    VKW::ResourcesController& controller = *device->GetResourcesController();
    for (std::uint8_t i = 0; i < buffering; i++)
    {
        AllocationContext& allocContext = m_AllocationContexts[i];

        allocContext.m_Buffer = controller.CreateBuffer(size, USAGE);
        allocContext.m_MappedBufferBegin = DRE::PtrAdd(allocContext.m_Buffer->GetMemoryPage()->mappedMemoryPtr_, allocContext.m_Buffer->memory_.offset_);
        allocContext.m_CurrentBufferPtr = allocContext.m_MappedBufferBegin;
    }
}

template<VKW::BufferUsage USAGE>
TransientArena<USAGE>::TransientArena(TransientArena<USAGE>&& rhs)
    : DeviceChild{ nullptr }
    , m_TransientBuffersSize{ 0 }
    , m_TransientBuffersCount{ 0 }
{
    operator=(DRE_MOVE(rhs));
}

template<VKW::BufferUsage USAGE>
TransientArena<USAGE>& TransientArena<USAGE>::operator=(TransientArena<USAGE>&& rhs)
{
    DeviceChild::operator=(DRE_MOVE(rhs));

    DRE_SWAP_MEMBER(m_TransientBuffersSize);
    DRE_SWAP_MEMBER(m_TransientBuffersCount);
    DRE_SWAP_MEMBER(m_AllocationContexts);

    return *this;
}

template<VKW::BufferUsage USAGE>
TransientArena<USAGE>::~TransientArena()
{
    VKW::ResourcesController& controller = *m_ParentDevice->GetResourcesController();
    for (std::uint8_t i = 0; i < m_TransientBuffersCount; i++)
    {
        controller.FreeBuffer(m_AllocationContexts[i].m_Buffer);
    }
}

template<VKW::BufferUsage USAGE>
typename TransientArena<USAGE>::Allocation TransientArena<USAGE>::AllocateTransientRegion(FrameID frameID, std::uint32_t size, std::uint32_t alignment)
{
    AllocationContext& allocationContext = m_AllocationContexts[frameID];

    DRE_ASSERT(DRE::IsPowOf2(alignment), "Alignment for TransientArena is not power of 2");

    std::uint32_t padding = static_cast<std::uint32_t>(DRE::PtrDifference(DRE::PtrAlign((std::uint8_t*)allocationContext.m_CurrentBufferPtr, alignment), allocationContext.m_CurrentBufferPtr));
    std::uint32_t const allocationSize = size + padding;
    DRE_ASSERT(allocationSize <= ComputeFreeMemory(allocationContext), "Out of transient staging memory");

    Allocation result{};
    result.m_Buffer         = allocationContext.m_Buffer;
    result.m_OffsetInBuffer = std::uint32_t(DRE::PtrDifference(DRE::PtrAlign(allocationContext.m_CurrentBufferPtr, alignment), allocationContext.m_MappedBufferBegin));
    result.m_Size           = size;
    result.m_MappedRange    = DRE::PtrAlign(allocationContext.m_CurrentBufferPtr, alignment);

    result.m_FrameID        = frameID;
    result.m_Arena          = this;


    allocationContext.m_CurrentBufferPtr = DRE::PtrAdd(allocationContext.m_CurrentBufferPtr, allocationSize);

    return result;
}

template<>
inline void TransientArena<VKW::BufferUsage::UPLOAD_BUFFER>::FlushCaches(typename Allocation const& allocation)
{
    AllocationContext& allocationContext = m_AllocationContexts[allocation.m_FrameID];

    if (allocationContext.m_Buffer->GetMemoryPage()->propertyFlags_ & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        return;

    VkMappedMemoryRange range{};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.pNext = nullptr;
    range.memory = allocationContext.m_Buffer->memory_.page_->deviceMemory_;
    range.offset = allocationContext.m_Buffer->memory_.offset_ + allocation.m_OffsetInBuffer;
    range.size   = allocation.m_Size;

    VK_ASSERT(m_ParentDevice->GetFuncTable()->vkFlushMappedMemoryRanges(m_ParentDevice->GetLogicalDevice()->Handle(), 1, &range));
}

template<>
inline void TransientArena<VKW::BufferUsage::UNIFORM>::FlushCaches(typename Allocation const& allocation)
{
    AllocationContext& allocationContext = m_AllocationContexts[allocation.m_FrameID];

    if (allocationContext.m_Buffer->GetMemoryPage()->propertyFlags_ & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        return;

    VkMappedMemoryRange range{};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.pNext = nullptr;
    range.memory = allocationContext.m_Buffer->memory_.page_->deviceMemory_;
    range.offset = allocationContext.m_Buffer->memory_.offset_ + allocation.m_OffsetInBuffer;
    range.size = allocation.m_Size;

    VK_ASSERT(m_ParentDevice->GetFuncTable()->vkFlushMappedMemoryRanges(m_ParentDevice->GetLogicalDevice()->Handle(), 1, &range));
}

template<>
inline void TransientArena<VKW::BufferUsage::READBACK_BUFFER>::FlushCaches(typename Allocation const& allocation)
{
    AllocationContext& allocationContext = m_AllocationContexts[allocation.m_FrameID];

    if(allocationContext.m_Buffer->GetMemoryPage()->propertyFlags_ & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        return;

    VkMappedMemoryRange range{};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.pNext = nullptr;
    range.memory = allocationContext.m_Buffer->memory_.page_->deviceMemory_;
    range.offset = allocationContext.m_Buffer->memory_.offset_ + allocation.m_OffsetInBuffer;
    range.size   = allocation.m_Size;

    VK_ASSERT(m_ParentDevice->GetFuncTable()->vkInvalidateMappedMemoryRanges(m_ParentDevice->GetLogicalDevice()->Handle(), 1, &range));
}


template<VKW::BufferUsage USAGE>
void TransientArena<USAGE>::InvalidateRanges(typename Allocation const& allocation)
{
    AllocationContext& allocationContext = m_AllocationContexts[allocation.m_FrameID];

    if (allocationContext.m_Buffer->GetMemoryPage()->propertyFlags_ & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        return;

    VkMappedMemoryRange range{};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.pNext = nullptr;
    range.memory = allocationContext.m_Buffer->memory_.page_->deviceMemory_;
    range.offset = allocationContext.m_Buffer->memory_.offset_ + allocation.m_OffsetInBuffer;
    range.size = allocation.m_Size;

    VK_ASSERT(m_ParentDevice->GetFuncTable()->vkInvalidateMappedMemoryRanges(m_ParentDevice->GetLogicalDevice()->Handle(), 1, &range));
}

template<VKW::BufferUsage USAGE>
std::uint32_t TransientArena<USAGE>::ComputeFreeMemory(AllocationContext const& context)
{
    return context.m_Buffer->size_ - static_cast<std::uint32_t>(DRE::PtrDifference(context.m_CurrentBufferPtr, context.m_MappedBufferBegin));
}

template<VKW::BufferUsage USAGE>
void TransientArena<USAGE>::ResetAllocations(FrameID frameID)
{
    AllocationContext& allocationContext = m_AllocationContexts[frameID];
    allocationContext.m_CurrentBufferPtr = allocationContext.m_MappedBufferBegin;
}

////////////////////////////////////////////
template<VKW::BufferUsage USAGE>
void TransientArena<USAGE>::Allocation::FlushCaches()
{
    m_Arena->FlushCaches(*this);
}

template<VKW::BufferUsage USAGE>
void TransientArena<USAGE>::Allocation::InvalidateRanges()
{
    m_Arena->InvalidateRanges(*this);
}

////////////////////////////////////////////
////////////////////////////////////////////
////////////////////////////////////////////


using UploadArena      = TransientArena<VKW::BufferUsage::UPLOAD_BUFFER>;
using ReadbackArena    = TransientArena<VKW::BufferUsage::READBACK_BUFFER>;
using UniformArena     = TransientArena<VKW::BufferUsage::UNIFORM>;


}

