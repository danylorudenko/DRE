#include <vk_wrapper\pipeline\Dependency.hpp>

#include <foundation\Common.hpp>
#include <vk_wrapper\Helper.hpp>

namespace VKW
{

VkAccessFlags2KHR AccessToFlags(ResourceAccess access)
{
    switch (access)
    {
    case RESOURCE_ACCESS_UNDEFINED:
    case RESOURCE_ACCESS_NONE:
        return VK_ACCESS_2_NONE;

    case RESOURCE_ACCESS_TRANSFER_DST:
        return VK_ACCESS_2_TRANSFER_WRITE_BIT;

    case RESOURCE_ACCESS_TRANSFER_SRC:
        return VK_ACCESS_2_TRANSFER_READ_BIT;

    case RESOURCE_ACCESS_COLOR_ATTACHMENT:
        return VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

    case RESOURCE_ACCESS_DEPTH_STENCIL_ATTACHMENT:
        return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

    case RESOURCE_ACCESS_SHADER_READ:
        return VK_ACCESS_2_SHADER_READ_BIT_KHR;

    case RESOURCE_ACCESS_SHADER_UNIFORM:
        return VK_ACCESS_2_UNIFORM_READ_BIT;

    case RESOURCE_ACCESS_SHADER_SAMPLE:
        return VK_ACCESS_2_SHADER_SAMPLED_READ_BIT_KHR;

    case RESOURCE_ACCESS_SHADER_WRITE:
        return VK_ACCESS_2_SHADER_WRITE_BIT;

    case RESOURCE_ACCESS_SHADER_RW:
        return VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_SHADER_READ_BIT;

    case RESOURCE_ACCESS_HOST_WRITE:
        return VK_ACCESS_2_HOST_WRITE_BIT;

    case RESOURCE_ACCESS_HOST_READ:
        return VK_ACCESS_2_HOST_READ_BIT;

    case RESOURCE_ACCESS_CLEAR:
        return VK_ACCESS_2_TRANSFER_WRITE_BIT;

    case RESOURCE_ACCESS_PRESENT:
    case RESOURCE_ACCESS_GENERIC_READ:
        return VK_ACCESS_2_MEMORY_READ_BIT;

    case RESOURCE_ACCESS_GENERIC_WRITE:
        return VK_ACCESS_2_MEMORY_WRITE_BIT;


    default:
        DRE_ASSERT(false, "Unsupported ResourceAccess.");
        return VK_FLAGS_NONE;
    }

    return VkAccessFlags2KHR(access);
}

VkImageLayout AccessToLayout(ResourceAccess access)
{
    switch (access)
    {
    case RESOURCE_ACCESS_UNDEFINED:
        return VK_IMAGE_LAYOUT_UNDEFINED;

    case RESOURCE_ACCESS_SHADER_READ:
    case RESOURCE_ACCESS_SHADER_SAMPLE:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    case RESOURCE_ACCESS_NONE:
    case RESOURCE_ACCESS_SHADER_WRITE:
    case RESOURCE_ACCESS_GENERIC_WRITE:
    case RESOURCE_ACCESS_SHADER_RW:
        return VK_IMAGE_LAYOUT_GENERAL;

    case RESOURCE_ACCESS_TRANSFER_DST:
    case RESOURCE_ACCESS_CLEAR:
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    case RESOURCE_ACCESS_TRANSFER_SRC:
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    case RESOURCE_ACCESS_COLOR_ATTACHMENT:
    case RESOURCE_ACCESS_DEPTH_STENCIL_ATTACHMENT:
        return VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;

    case RESOURCE_ACCESS_PRESENT:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    case RESOURCE_ACCESS_GENERIC_READ:
        VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;

    default:
        DRE_ASSERT(false, "Unsupported ResourceAccess");
        return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

VkPipelineStageFlags2 StagesToFlags(Stages stage)
{
    /*
    switch (stage)
    {
    case STAGE_INPUT_ASSEMBLER:
        return VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT_KHR;
    case STAGE_VERTEX:
        return VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT_KHR;
    case STAGE_FRAGMENT:
        return VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR;
    case STAGE_COLOR_OUTPUT:
        return VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    case STAGE_COMPUTE:
        return VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;
    case STAGE_TRANSFER:
        return VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR;
    case STAGE_HOST:
        return VK_PIPELINE_STAGE_2_HOST_BIT_KHR;
    case STAGE_PRESENT:
    case STAGE_TOP:
        return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    default:
        return VK_PIPELINE_STAGE_2_NONE_KHR;
    }*/
    return VkPipelineStageFlags2{ stage };
}


//inline bool BarrierRequirements(VKW::ResourceAccess prevAccess, VKW::ResourceAccess access, bool& requireExecutionDependency, bool& requireMemoryDependency, bool& requireTransition)
//{
//    // just reading, no hazards
//    if((prevAccess & VKW::RESOURCE_ACCESS_ANY_READ) && (access & VKW::RESOURCE_ACCESS_ANY_READ))
//        return false;
//
//    if(access & VKW::RESOURCE_ACCESS_ANY_ATTACHMENT)
//    {
//        requireExecutionDependency = true;
//        requireMemoryDependency    = true;
//        requireTransition          = true;
//
//        return true;
//    }
//
//    // just execution dependency, no need for cache flushing
//    if ((prevAccess & VKW::RESOURCE_ACCESS_ANY_READ) && (access & VKW::RESOURCE_ACCESS_ANY_WRITE))
//    {
//        requireExecutionDependency  = true;
//        return true;
//    }
//
//    // WAW hazard, need memory dependency to prevent reordering
//    if ((prevAccess & VKW::RESOURCE_ACCESS_ANY_WRITE) && (access & VKW::RESOURCE_ACCESS_ANY_WRITE))
//    {
//        requireExecutionDependency  = true;
//        requireMemoryDependency     = true;
//        return true;
//    }
//
//    // after transfers we most likely need resource in another state, memory dependency
//    if ((prevAccess & VKW::RESOURCE_ACCESS_ANY_TRANSFER))
//    {
//        requireExecutionDependency  = true;
//        requireMemoryDependency     = true;
//        requireTransition           = true;
//        return true;
//    }
//
//    return false;
//}
//
//inline bool BarrierRequirements(VKW::ResourceAccess prevAccess, VKW::ResourceAccess access, bool& requireExecutionDependency, bool& requireMemoryDependency)
//{
//    // casually reading, no hazards
//    if((prevAccess & VKW::RESOURCE_ACCESS_ANY_READ) && (access & VKW::RESOURCE_ACCESS_ANY_READ))
//        return false;
//
//    if ((prevAccess & VKW::RESOURCE_ACCESS_ANY_READ) && (access & VKW::RESOURCE_ACCESS_ANY_WRITE))
//    {
//        requireExecutionDependency = true;
//        return true;
//    }
//
//    if ((prevAccess & VKW::RESOURCE_ACCESS_ANY_WRITE) && (access & VKW::RESOURCE_ACCESS_ANY_WRITE))
//    {
//        requireExecutionDependency = true;
//        requireMemoryDependency    = true;
//        return true;
//    }
//
//    if ((prevAccess & VKW::RESOURCE_ACCESS_ANY_WRITE) && (access & VKW::RESOURCE_ACCESS_ANY_READ))
//    {
//        requireExecutionDependency = true;
//        requireMemoryDependency    = true;
//        return true;
//    }
//
//    return false;
//}

Dependency::Dependency()
    : memoryBarriers_{}
    , bufferBarriers_{}
    , imageBarriers_{}
{}

Dependency::Dependency(DRE::AllocatorLinear* allocator)
    : memoryBarriers_{ allocator }
    , bufferBarriers_{ allocator }
    , imageBarriers_{ allocator }
{}

void Dependency::Add(
    VKW::ImageResource const* resource, 
    ResourceAccess srcAccess, Stages srcStage, std::uint32_t srcQueueFamily,
    ResourceAccess dstAccess, Stages dstStage, std::uint32_t dstQueueFamily)
{
    VkImageMemoryBarrier2KHR& barrier = imageBarriers_.EmplaceBack();
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
    barrier.pNext = nullptr;
    barrier.srcStageMask    = StagesToFlags(srcStage);
    barrier.srcAccessMask   = AccessToFlags(srcAccess);
    barrier.dstStageMask    = StagesToFlags(dstStage);
    barrier.dstAccessMask   = AccessToFlags(dstAccess);
    barrier.oldLayout       = AccessToLayout(srcAccess);
    barrier.newLayout       = AccessToLayout(dstAccess);
    barrier.image           = resource->handle_;
    barrier.subresourceRange = HELPER::DefaultImageSubresourceRange();
}

void Dependency::Add(
    VKW::BufferResource const* resource,
    ResourceAccess srcAccess, Stages srcStage, std::uint32_t srcQueueFamily,
    ResourceAccess dstAccess, Stages dstStage, std::uint32_t dstQueueFamily)
{
    Add(resource,
        0, resource->size_,
        srcAccess, srcStage, srcQueueFamily,
        dstAccess, dstStage, dstQueueFamily
    );
}

void Dependency::Add(
    VKW::BufferResource const* resource,
    std::uint64_t offset, std::uint32_t size,
    ResourceAccess srcAccess, Stages srcStage, std::uint32_t srcQueueFamily,
    ResourceAccess dstAccess, Stages dstStage, std::uint32_t dstQueueFamily)
{
    if (srcQueueFamily == dstQueueFamily)
    {
        VkMemoryBarrier2KHR& barrier = memoryBarriers_.EmplaceBack();
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2_KHR;
        barrier.pNext = nullptr;
        barrier.srcStageMask  = StagesToFlags(srcStage);
        barrier.srcAccessMask = AccessToFlags(srcAccess);
        barrier.dstStageMask  = StagesToFlags(dstStage);
        barrier.dstAccessMask = AccessToFlags(dstAccess);
    }
    else
    {
        VkBufferMemoryBarrier2KHR& barrier = bufferBarriers_.EmplaceBack();
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR;
        barrier.pNext = nullptr;
        barrier.srcStageMask    = StagesToFlags(srcStage);
        barrier.srcAccessMask   = AccessToFlags(srcAccess);
        barrier.dstStageMask    = StagesToFlags(dstStage);
        barrier.dstAccessMask   = AccessToFlags(dstAccess);
        barrier.srcQueueFamilyIndex = srcQueueFamily;
        barrier.dstQueueFamilyIndex = dstQueueFamily;
        barrier.buffer = resource->handle_;
        barrier.offset = offset;
        barrier.size   = size;
    }
}

void Dependency::MergeWith(Dependency const& rhs)
{
    for (std::uint32_t i = 0; i < rhs.memoryBarriers_.Size(); i++)
    {
        memoryBarriers_.EmplaceBack(rhs.memoryBarriers_[i]);
    }

    for (std::uint32_t i = 0; i < rhs.bufferBarriers_.Size(); i++)
    {
        bufferBarriers_.EmplaceBack(rhs.bufferBarriers_[i]);
    }

    for (std::uint32_t i = 0; i < rhs.imageBarriers_.Size(); i++)
    {
        imageBarriers_.EmplaceBack(rhs.imageBarriers_[i]);
    }
}

void Dependency::Clear()
{
    memoryBarriers_.Clear();
    bufferBarriers_.Clear();
    imageBarriers_.Clear();
}

bool Dependency::IsEmpty() const
{
    return 
        memoryBarriers_.Empty() &&
        bufferBarriers_.Empty() &&
        imageBarriers_.Empty();
}

void Dependency::GetDependency(VkDependencyInfoKHR& result)
{
    result.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
    result.pNext = nullptr;
    result.dependencyFlags = VK_FLAGS_NONE;
    result.memoryBarrierCount          = memoryBarriers_.Size();
    result.pMemoryBarriers             = memoryBarriers_.Data();
    result.bufferMemoryBarrierCount    = bufferBarriers_.Size();
    result.pBufferMemoryBarriers       = bufferBarriers_.Data();
    result.imageMemoryBarrierCount     = imageBarriers_.Size();
    result.pImageMemoryBarriers        = imageBarriers_.Data();

}

VKW::DescriptorStage StageToDescriptorStage(VKW::Stages stage)
{
    std::uint16_t result = VKW::DESCRIPTOR_STAGE_NONE;

    if (stage & VKW::STAGE_COMPUTE)
        result |= VKW::DESCRIPTOR_STAGE_COMPUTE;

    if (stage & VKW::STAGE_VERTEX)
        result |= VKW::DESCRIPTOR_STAGE_VERTEX;

    if (stage & VKW::STAGE_FRAGMENT)
        result |= VKW::DESCRIPTOR_STAGE_FRAGMENT;

    return VKW::DescriptorStage(result);
}

}

