#pragma once

#include <cstdint>
#include <vulkan\vulkan.h>

#include <foundation\memory\AllocatorLinear.hpp>

#include <foundation\class_features\NonMovable.hpp>
#include <foundation\class_features\NonCopyable.hpp>

#include <foundation\Container\Vector.hpp>

#include <vk_wrapper\Tools.hpp>
#include <vk_wrapper\descriptor\Descriptor.hpp>
#include <vk_wrapper\resources\Resource.hpp>

namespace VKW
{

/////////////////////////////////////
enum ResourceAccessBits : std::uint64_t
{
    RESOURCE_ACCESS_UNDEFINED                   = DRE_U64_MAX,
    RESOURCE_ACCESS_NONE                        = VK_ACCESS_2_NONE,
    RESOURCE_ACCESS_TRANSFER_DST                = VK_ACCESS_2_TRANSFER_WRITE_BIT,
    RESOURCE_ACCESS_TRANSFER_SRC                = VK_ACCESS_2_TRANSFER_READ_BIT,
    RESOURCE_ACCESS_COLOR_ATTACHMENT            = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
    RESOURCE_ACCESS_DEPTH_STENCIL_ATTACHMENT    = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    RESOURCE_ACCESS_SHADER_READ                 = VK_ACCESS_2_SHADER_READ_BIT,
    RESOURCE_ACCESS_SHADER_UNIFORM              = VK_ACCESS_2_UNIFORM_READ_BIT,
    RESOURCE_ACCESS_SHADER_SAMPLE               = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
    RESOURCE_ACCESS_SHADER_WRITE                = VK_ACCESS_2_SHADER_WRITE_BIT,
    RESOURCE_ACCESS_HOST_WRITE                  = VK_ACCESS_2_HOST_WRITE_BIT,
    RESOURCE_ACCESS_HOST_READ                   = VK_ACCESS_2_HOST_READ_BIT,
    RESOURCE_ACCESS_CLEAR                       = RESOURCE_ACCESS_TRANSFER_DST,
    RESOURCE_ACCESS_PRESENT                     = VK_ACCESS_2_MEMORY_READ_BIT,


    RESOURCE_ACCESS_SHADER_RW         = RESOURCE_ACCESS_SHADER_READ | RESOURCE_ACCESS_SHADER_WRITE,
    RESOURCE_ACCESS_ANY_TRANSFER      = RESOURCE_ACCESS_TRANSFER_SRC | RESOURCE_ACCESS_TRANSFER_DST,
    RESOURCE_ACCESS_ANY_ATTACHMENT    = RESOURCE_ACCESS_COLOR_ATTACHMENT | RESOURCE_ACCESS_DEPTH_STENCIL_ATTACHMENT,
    RESOURCE_ACCESS_ANY_WRITE         = RESOURCE_ACCESS_SHADER_WRITE | RESOURCE_ACCESS_TRANSFER_DST | RESOURCE_ACCESS_ANY_ATTACHMENT,
    RESOURCE_ACCESS_ANY_READ          = RESOURCE_ACCESS_SHADER_READ | RESOURCE_ACCESS_SHADER_UNIFORM | RESOURCE_ACCESS_TRANSFER_SRC | RESOURCE_ACCESS_SHADER_SAMPLE | RESOURCE_ACCESS_PRESENT
};
using ResourceAccess = std::uint64_t;

/////////////////////////////////////
enum StageBits : std::uint64_t
{
    STAGE_UNDEFINED         = 0,
    STAGE_INPUT_ASSEMBLER   = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT_KHR,
    STAGE_VERTEX            = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT_KHR,
    STAGE_FRAGMENT          = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR,
    STAGE_COLOR_OUTPUT      = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    STAGE_COMPUTE           = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR,
    STAGE_TRANSFER          = VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR,
    STAGE_HOST              = VK_PIPELINE_STAGE_2_HOST_BIT_KHR,
    STAGE_TOP               = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
    STAGE_BOTTOM            = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
    STAGE_PRESENT           = STAGE_TOP,
};
using Stages = std::uint64_t;


VkAccessFlags2KHR       AccessToFlags(ResourceAccess access);
VkImageLayout           AccessToLayout(ResourceAccess access);
VkPipelineStageFlags2   StagesToFlags(Stages stage);


bool BarrierRequirements(VKW::ResourceAccess prevAccess, VKW::ResourceAccess access, bool& requireExecutionDependency, bool& requireMemoryDependency, bool& requireTransition);
bool BarrierRequirements(VKW::ResourceAccess prevAccess, VKW::ResourceAccess access, bool& requireExecutionDependency, bool& requireMemoryDependency);

class Dependency
    : public NonMovable
    , public NonCopyable
{
public:
    Dependency();
    Dependency(DRE::AllocatorLinear* allocator);

    void Add(VKW::ImageResource const* resource,
        ResourceAccess srcAccess, Stages srcStage, std::uint32_t srcQueueFamily,
        ResourceAccess dstAccess, Stages dstStage, std::uint32_t dstQueueFamily);

    void Add(VKW::BufferResource const* resource,
        ResourceAccess srcAccess, Stages srcStage, std::uint32_t srcQueueFamily,
        ResourceAccess dstAccess, Stages dstStage, std::uint32_t dstQueueFamily);

    void Add(VKW::BufferResource const* resource,
        std::uint64_t offset, std::uint32_t size, // offset is added to the base offset of the VKW::BufferResource
        ResourceAccess srcAccess, Stages srcStage, std::uint32_t srcQueueFamily,
        ResourceAccess dstAccess, Stages dstStage, std::uint32_t dstQueueFamily);

    void GetDependency(VkDependencyInfoKHR& result);

    void MergeWith(Dependency const& rhs);

    void Clear();
    bool IsEmpty() const;

private:
    DRE::Vector<VkMemoryBarrier2KHR,        DRE::AllocatorLinear>   memoryBarriers_;
    DRE::Vector<VkBufferMemoryBarrier2KHR,  DRE::AllocatorLinear>   bufferBarriers_;
    DRE::Vector<VkImageMemoryBarrier2KHR,   DRE::AllocatorLinear>   imageBarriers_;
};

VKW::DescriptorStage StageToDescriptorStage(VKW::Stages stage);

}

