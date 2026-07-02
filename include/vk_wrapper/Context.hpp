#pragma once

#include <cstdint>

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>

#include <foundation\memory\AllocatorLinear.hpp>
#include <foundation\memory\ByteBuffer.hpp>
#include <foundation\Container\Vector.hpp>

#include <vk_wrapper\queue\Queue.hpp>
#include <vk_wrapper\descriptor\Descriptor.hpp>
#include <vk_wrapper\pipeline\Dependency.hpp>

#include <common\shaders_defines.slang>

namespace VKW
{
class   ImportTable;
class   Device;
class   Queue;
struct  BufferResource;
class   Pipeline;
class   DescriptorSetLayout;
class   PipelineLayout;
class   RenderPass;
class   Framebuffer;
class   Dependency;
class   DescriptorManager;

enum class BindPoint
{
    Graphics,
    Compute
};

enum AttachmentMaskBits
{
    ATTACHMENT_MASK_DEPTH   = 1,
    ATTACHMENT_MASK_STENCIL = 1 << 1,
    ATTACHMENT_MASK_COLOR_0 = 1 << 2,
    ATTACHMENT_MASK_COLOR_1 = 1 << 3,
    ATTACHMENT_MASK_COLOR_2 = 1 << 4,
    ATTACHMENT_MASK_COLOR_3 = 1 << 5,
    ATTACHMENT_MASK_COLOR_4 = 1 << 6
};

enum PolygonModeBits
{
    POLYGON_FILL,
    POLYGON_WIREFRAME
};

using AttachmentMask = DRE::U32;

////////////////////////////////////////
////////////////////////////////////////
////////////////////////////////////////

class Context
    : public NonMovable
    , public NonCopyable
{
public:
    Context(VKW::ImportTable* table, VKW::Queue* queue, DRE::AllocatorLinear* barrierAllocator);
    ~Context();

public:
    inline VKW::Queue* GetParentQueue() const { return m_ParentQueue; }
    inline VKW::CommandList* GetCurrentCommandList() { return m_CurrentCommandList; }

    void ResetDependenciesVectors(DRE::AllocatorLinear* allocator);
    
    void FlushAll();
    void FlushOnlyPending();
    void WriteResourceDependencies();

    void FlushWaitSwapchain(PresentationContext& presentContext);
    void Present(PresentationContext& presentContext);

    VKW::QueueExecutionPoint SyncPoint(DRE::U8 waitCount = 0, VKW::QueueExecutionPoint const* waits = nullptr);
    VKW::QueueExecutionPoint SyncPoint(VKW::QueueExecutionPoint const& wait);

public:
    void CmdDraw(DRE::U32 vertexCount, DRE::U32 instanceCount = 1, DRE::U32 firstVertex = 0, DRE::U32 firstInstance = 0);
    void CmdDrawIndirect(VKW::BufferResource const* buffer, DRE::U32 offset = 0, DRE::U32 drawCount = 1, DRE::U32 stride = sizeof(DrawIndirectCommand));
    void CmdDrawIndexed(DRE::U32 indexCount, DRE::U32 instanceCount = 1, DRE::U32 firstIndex = 0, DRE::S32 vertexOffset = 0, DRE::U32 firstInstance = 0);
    void CmdDrawIndexedIndirect(VKW::BufferResource const* buffer, DRE::U32 offset = 0, DRE::U32 drawCount = 1, DRE::U32 stride = sizeof(DrawIndexedIndirectCommand));
    void CmdDispatch(DRE::U32 x, DRE::U32 y, DRE::U32 z);

    void CmdBindPipeline(BindPoint bindPoint, VKW::Pipeline const* pipeline);
    void CmdBindGraphicsPipeline(VKW::Pipeline const* pipeline);
    void CmdBindComputePipeline(VKW::Pipeline const* pipeline);

    void CmdBindDescriptorSets(
        VKW::PipelineLayout const* layout, BindPoint bindPoint,
        DRE::U32 firstSet, DRE::U32 descriptorSetCount, VKW::DescriptorSet const* sets,
        DRE::U32 dynamicOffsetCount = 0, DRE::U32 const* pDynamicOffsets = nullptr);

    void CmdBindGraphicsDescriptorSets(
        VKW::PipelineLayout const* layout, 
        DRE::U32 firstSet, DRE::U32 descriptorSetCount, VKW::DescriptorSet const* sets,
        DRE::U32 dynamicOffsetCount = 0, DRE::U32 const* pDynamicOffsets = nullptr);

    void CmdBindComputeDescriptorSets(
        VKW::PipelineLayout const* layout,
        DRE::U32 firstSet, DRE::U32 descriptorSetCount, VKW::DescriptorSet const* sets,
        DRE::U32 dynamicOffsetCount = 0, DRE::U32 const* pDynamicOffsets = nullptr);

    void CmdBindGlobalDescriptorSets(VKW::DescriptorManager& descriptorManager, DRE::U8 frameID);

    void CmdSetViewport(DRE::U32 viewportCount, DRE::U32 x, DRE::U32 y, DRE::U32 width, DRE::U32 height);
    void CmdSetScissor(DRE::U32 scissorCount, DRE::U32 x, DRE::U32 y, DRE::U32 width, DRE::U32 height);

#ifndef DRE_COMPILE_FOR_RENDERDOC
    void CmdSetPolygonMode(PolygonModeBits mode);
#endif // DRE_COMPILE_FOR_RENDERDOC

    void CmdPushConstants(VKW::PipelineLayout const* layout, VKW::DescriptorStage stages, DRE::U32 offset, DRE::U32 size, void const* pValues);

    // very heavy
    //void CmdPipelineBarrier(VKW::Dependency& dependency);

    void CmdMemoryDependency(
        ResourceAccess srcAccess, Stages srcStage,
        ResourceAccess dstAccess, Stages dstStage
    );

    void CmdResourceDependency(VKW::ImageResource const* resource,
        ResourceAccess srcAccess, Stages srcStage,
        ResourceAccess dstAccess, Stages dstStage);

    void CmdResourceDependency(VKW::BufferResource const* resource,
        ResourceAccess srcAccess, Stages srcStage,
        ResourceAccess dstAccess, Stages dstStage);

    void CmdResourceDependency(VKW::BufferResource const* resource,
        DRE::U32 offset, DRE::U32 size, // offset is added to the base offset of the VKW::BufferResource
        ResourceAccess srcAccess, Stages srcStage,
        ResourceAccess dstAccess, Stages dstStage);


    void CmdBeginRendering(DRE::U32 attachmentCount, VKW::ImageResourceView* const* attachments, VKW::ImageResourceView const* depthAttachment, VKW::ImageResourceView const* stencilAttachment);
    void CmdClearAttachments(AttachmentMask attachments, float* color);
    void CmdClearAttachments(AttachmentMask attachments, DRE::U32* value);
    void CmdClearAttachments(AttachmentMask attachments, float depth, DRE::U32 stencil);
    void CmdEndRendering();

    void CmdBindVertexBuffer(VKW::BufferResource const* vertexBuffer, DRE::U32 offset = 0);
    void CmdBindIndexBuffer(VKW::BufferResource const* indexBuffer, DRE::U32 offset = 0, DRE::U8 indexSize = 32);

    void CmdClearColorImage(VKW::ImageResource const* image, float color[4]);
    void CmdClearDepthStencilImage(VKW::ImageResource const* image, float depth, DRE::U32 stencil);

    void CmdCopyImageToImage(VKW::ImageResource const* dst, VKW::ImageResource const* src);
    void CmdCopyImageToBuffer(VKW::BufferResource const* dst, VKW::ImageResource const* src, DRE::U32 bufferOffset);
    void CmdCopyBufferToImage(VKW::ImageResource const* dst, VKW::BufferResource const* src, DRE::U32 bufferOffset);
    void CmdCopyBufferToBuffer(VKW::BufferResource const* dst, DRE::U32 dstOffset, VKW::BufferResource const* scr, DRE::U32 srcOffset, DRE::U32 size);
    void CmdFillBuffer(VKW::BufferResource const* dst, DRE::U32 dstOffset, DRE::U32 size, DRE::U32 data);
    void CmdUpdateBuffer(VKW::BufferResource const* dst, DRE::U32 dstOffset, DRE::U32 size, void const* pData);

    void CmdBuildBLAS(
        VKW::AccelerationStructureResource const* blas,
        DRE::U64 scratchBufferAddress,
        DRE::U64 vertexBufferAddress,
        DRE::U64 vertexStride,
        DRE::U64 vertexCount,
        DRE::U64 indexBufferAddress,
        DRE::U64 indexCount);

    void CmdBuildTLAS(
        VKW::AccelerationStructureResource const* tlas,
        DRE::U64 scratchBufferAddress,
        DRE::U32 instanceCount,
        DRE::U64 instanceBufferAddress);

    void CmdBeginDebugLabel(char const* label);
    void CmdEndDebugLabel();
    void CmdInsertDebugLabel(char const* label);

    void WaitIdle();

    static bool TestLayoutCompatibility(VKW::PipelineLayout const* parentLayout, VKW::PipelineLayout const* childLayout);

private:
    VKW::ImportTable*       m_ImportTable;
    VKW::Queue*             m_ParentQueue;
    VKW::CommandList*       m_CurrentCommandList;

    VkRect2D                m_RenderingRect;

    VKW::Dependency         m_PendingDependency;

};


///////////////////////////////
class _GPU_DEBUG_SCOPE_
{
public:
    _GPU_DEBUG_SCOPE_(Context& context, char const* name)
        : m_Context{ &context }
    {
        m_Context->CmdBeginDebugLabel(name);
    }

    ~_GPU_DEBUG_SCOPE_()
    {
        m_Context->CmdEndDebugLabel();
    }

private:
    Context* m_Context;
};
#define DRE_GPU_SCOPE(Name) VKW::_GPU_DEBUG_SCOPE_ _##Name##_GPU_SCOPE{ context, #Name }
#define DRE_GPU_EVENT(Name) context.CmdInsertDebugLabel(#Name)


}

