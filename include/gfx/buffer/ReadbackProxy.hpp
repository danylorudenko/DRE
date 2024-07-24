#pragma once

#include <vk_wrapper\Context.hpp>
#include <vk_wrapper\queue\Queue.hpp>

#include <gfx\buffer\TransientArena.hpp>

namespace GFX
{

// should be hold as a member for future use
class ReadbackFuture
{
private:
    friend class ReadbackScheduler;
    ReadbackFuture(VKW::QueueExecutionPoint const& executionPoint, ReadbackArena::Allocation const& allocation);

public:
    ReadbackFuture();
    ReadbackFuture(ReadbackFuture const& rhs);
    ReadbackFuture(ReadbackFuture&& rhs);

    ReadbackFuture& operator=(ReadbackFuture const& rhs);
    ReadbackFuture& operator=(ReadbackFuture&& rhs);

    void SyncRead(void* dst, std::uint32_t size);
    void Sync();
    void Read(void* dst, std::uint32_t size);

    explicit operator bool() { return static_cast<bool>(m_ExecutionPoint); }

    void* GetMappedPtr();

    ~ReadbackFuture();

private:
    VKW::QueueExecutionPoint    m_ExecutionPoint;
    ReadbackArena::Allocation   m_ReadbackAllocation;
};

//////////////////////////////////////
//////////////////////////////////////
//////////////////////////////////////

// should be created on stack
class ReadbackScheduler
    : public NonCopyable
    , public NonMovable
{
public:
    ReadbackScheduler(FrameID frameID, ReadbackArena* arena, std::uint32_t size);

    VKW::BufferResource*    GetDstBuffer() const;
    std::uint32_t           GetDstOffset() const;
    std::uint32_t           GetDstSize() const;

    ReadbackFuture CreateReadbackFuture(VKW::QueueExecutionPoint const& executionPoint) const;

private:
    ReadbackArena::Allocation   m_ReadbackAllocation;

};

}

