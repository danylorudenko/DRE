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
enum ResourceAccess : DRE::U64
{
    RESOURCE_ACCESS_UNDEFINED                       = 0,
    RESOURCE_ACCESS_NONE                            = (1 << 0),
    RESOURCE_ACCESS_TRANSFER_DST                    = (1 << 1),
    RESOURCE_ACCESS_TRANSFER_SRC                    = (1 << 2),
    RESOURCE_ACCESS_COLOR_ATTACHMENT                = (1 << 3),
    RESOURCE_ACCESS_DEPTH_ONLY_ATTACHMENT           = (1 << 4),
    RESOURCE_ACCESS_DEPTH_STENCIL_ATTACHMENT        = (1 << 5),
    RESOURCE_ACCESS_SHADER_READ                     = (1 << 6),
    RESOURCE_ACCESS_SHADER_UNIFORM                  = (1 << 7),
    RESOURCE_ACCESS_SHADER_SAMPLE                   = (1 << 8),
    RESOURCE_ACCESS_SHADER_WRITE                    = (1 << 9),
    RESOURCE_ACCESS_SHADER_RW                       = (1 << 10),
    RESOURCE_ACCESS_HOST_WRITE                      = (1 << 11),
    RESOURCE_ACCESS_HOST_READ                       = (1 << 12),
    RESOURCE_ACCESS_CLEAR                           = (1 << 13),
    RESOURCE_ACCESS_PRESENT                         = (1 << 14),
    RESOURCE_ACCESS_GENERIC_READ                    = (1 << 15),
    RESOURCE_ACCESS_GENERIC_WRITE                   = (1 << 16),
    RESOURCE_ACCESS_GENERIC_RW                      = (1 << 17),
    RESOURCE_ACCESS_INDIRECT_ARGS                   = (1 << 18),
    RESOURCE_ACCESS_ACCELERATION_STRUCTURE_TRACE    = (1 << 19),
    RESOURCE_ACCESS_ACCELERATION_STRUCTURE_BUILD    = (1 << 20),

    RESOURCE_ACCESS_INVALID                     = 0xFFFFFFFFFFFFFFFF,
};

/////////////////////////////////////
enum StageBits : DRE::U64
{
    STAGE_UNDEFINED         = 0,
    STAGE_INDIRECT_ARGS     = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT,
    STAGE_INPUT_ASSEMBLER   = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT_KHR,
    STAGE_VERTEX            = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT_KHR,
    STAGE_FRAGMENT          = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR,
    STAGE_COLOR_OUTPUT      = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    STAGE_COMPUTE           = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR,
    STAGE_TRANSFER          = VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR,
    STAGE_RAY_TRACE         = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
    STAGE_AS_BUILD          = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
    STAGE_HOST              = VK_PIPELINE_STAGE_2_HOST_BIT_KHR,
    STAGE_TOP               = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
    STAGE_BOTTOM            = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
    STAGE_ALL_GRAPHICS      = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
    STAGE_ALL_GLOBAL        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
    STAGE_PRESENT           = STAGE_TOP,
};
using Stages = DRE::U64;


VkAccessFlags2KHR       AccessToFlags(ResourceAccess access);
VkImageLayout           AccessToLayout(ResourceAccess access);
VkPipelineStageFlags2   StagesToFlags(Stages stage);


//bool BarrierRequirements(VKW::ResourceAccess prevAccess, VKW::ResourceAccess access, bool& requireExecutionDependency, bool& requireMemoryDependency, bool& requireTransition);
//bool BarrierRequirements(VKW::ResourceAccess prevAccess, VKW::ResourceAccess access, bool& requireExecutionDependency, bool& requireMemoryDependency);

class Dependency
    : public NonMovable
    , public NonCopyable
{
public:
    Dependency();
    Dependency(DRE::AllocatorLinear* allocator);

    void Add(
        ResourceAccess srcAccess, Stages srcStage,
        ResourceAccess dstAccess, Stages dstStage);

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

    void Reset(DRE::AllocatorLinear* allocator);
    void Clear();
    bool IsEmpty() const;

private:
    DRE::Vector<VkMemoryBarrier2KHR,        DRE::AllocatorLinear>   memoryBarriers_;
    DRE::Vector<VkBufferMemoryBarrier2KHR,  DRE::AllocatorLinear>   bufferBarriers_;
    DRE::Vector<VkImageMemoryBarrier2KHR,   DRE::AllocatorLinear>   imageBarriers_;
};

VKW::DescriptorStage StageToDescriptorStage(VKW::Stages stage);

}

