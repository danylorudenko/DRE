#include <vk_interface\Context.hpp>
#include <vk_interface\queue\Queue.hpp>

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

    ReadbackFuture CreateReadbackFuture(VKW::QueueExecutionPoint const& executionPoint) const;

private:
    ReadbackArena::Allocation   m_ReadbackAllocation;

};

}

