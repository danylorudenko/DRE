#include <gfx\buffer\ReadbackProxy.hpp>

namespace GFX
{

ReadbackFuture::ReadbackFuture()
    : m_ExecutionPoint{}
    , m_ReadbackAllocation{}
{}

ReadbackFuture::ReadbackFuture(VKW::QueueExecutionPoint const& executionPoint, ReadbackArena::Allocation const& allocation)
    : m_ExecutionPoint{ executionPoint }
    , m_ReadbackAllocation{ allocation }
{}

ReadbackFuture::ReadbackFuture(ReadbackFuture const&)               = default;
ReadbackFuture::ReadbackFuture(ReadbackFuture&&)                    = default;
ReadbackFuture& ReadbackFuture::operator=(ReadbackFuture const&)    = default;
ReadbackFuture& ReadbackFuture::operator=(ReadbackFuture&&)         = default;
ReadbackFuture::~ReadbackFuture()                                   = default;

void ReadbackFuture::Sync()
{
    m_ExecutionPoint.Wait();
}

void ReadbackFuture::Read(void* dst, std::uint32_t size)
{
    DRE_ASSERT(size <= m_ReadbackAllocation.m_Size, "Attempt to readback more than allocation allows.");
    
    m_ReadbackAllocation.InvalidateRanges();
    std::memcpy(dst, m_ReadbackAllocation.m_MappedRange, size);
}

void ReadbackFuture::SyncRead(void* dst, std::uint32_t size)
{
    Sync();
    Read(dst, size);
}


//////////////////////////////////////
//////////////////////////////////////
//////////////////////////////////////

ReadbackScheduler::ReadbackScheduler(FrameID frameID, ReadbackArena* arena, std::uint32_t size)
    : m_ReadbackAllocation{ arena->AllocateTransientRegion(frameID, size, 16) }
{
}

VKW::BufferResource* ReadbackScheduler::GetDstBuffer() const
{
    return m_ReadbackAllocation.m_Buffer;
}

std::uint32_t ReadbackScheduler::GetDstOffset() const
{
    return m_ReadbackAllocation.m_OffsetInBuffer;
}

std::uint32_t ReadbackScheduler::GetDstSize() const
{
    return m_ReadbackAllocation.m_Size;
}

ReadbackFuture ReadbackScheduler::CreateReadbackFuture(VKW::QueueExecutionPoint const& executionPoint) const
{
    return ReadbackFuture{ executionPoint, m_ReadbackAllocation };
}

}

