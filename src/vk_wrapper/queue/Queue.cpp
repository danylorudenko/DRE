#include <vk_wrapper\queue\Queue.hpp>

#include <utility>

#include <vk_wrapper\Tools.hpp>

namespace VKW
{

constexpr std::uint32_t INVALID_QUEUE = UINT32_MAX;

///////////////////////////////////////////
QueueExecutionPoint::QueueExecutionPoint()
    : point_{ 0 }
    , parentQueue_{ nullptr }
{}

QueueExecutionPoint::QueueExecutionPoint(Queue* parentQueue, std::uint64_t point)
    : point_{ point }
    , parentQueue_{ parentQueue }
{}

void QueueExecutionPoint::Wait()
{
    parentQueue_->WaitFor(*this);
}

VkSemaphore QueueExecutionPoint::GetTimelineSemaphore() const 
{ 
    return parentQueue_->GetTimelineSemaphore();
}

///////////////////////////////////////////
CommandList::CommandList(ImportTable* table, LogicalDevice* device, Queue* parentQueue)
    : table_{ table }
    , device_{ device }
    , commandBuffer_{ VK_NULL_HANDLE }
    , commandPool_{ VK_NULL_HANDLE }
    , executionFinishPoint_{ parentQueue, 0 }
    , parentQueue_{ parentQueue }
{
    DRE_DEBUG_ONLY(isOpened_ = false);

    VkCommandPoolCreateInfo poolInfo;
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.pNext = nullptr;
    poolInfo.flags = VK_FLAGS_NONE;
    poolInfo.queueFamilyIndex = parentQueue->GetQueueFamily();
    VK_ASSERT(table_->vkCreateCommandPool(device_->Handle(), &poolInfo, nullptr, &commandPool_));


    VkCommandBufferAllocateInfo allocInfo;
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    VK_ASSERT(table_->vkAllocateCommandBuffers(device_->Handle(), &allocInfo, &commandBuffer_));
}

CommandList::CommandList(CommandList&& rhs)
    : table_{ nullptr }
    , device_{ nullptr }
    , commandBuffer_{ VK_NULL_HANDLE }
    , commandPool_{ VK_NULL_HANDLE }
    , executionFinishPoint_{ nullptr, 0 }
    , parentQueue_{ nullptr }
{
    DRE_DEBUG_ONLY(isOpened_ = false);

    operator=(DRE_MOVE(rhs));
}

CommandList& CommandList::operator=(CommandList&& rhs)
{
    DRE_SWAP_MEMBER(table_);
    DRE_SWAP_MEMBER(device_);
    
    DRE_SWAP_MEMBER(commandBuffer_);
    DRE_SWAP_MEMBER(commandPool_);
    DRE_SWAP_MEMBER(executionFinishPoint_);

    DRE_SWAP_MEMBER(parentQueue_);

    DRE_DEBUG_ONLY( DRE_SWAP_MEMBER(isOpened_) );

    return *this;
}

CommandList::~CommandList()
{
    if (commandPool_ != VK_NULL_HANDLE)
    {
        table_->vkDestroyCommandPool(device_->Handle(), commandPool_, nullptr);
    }
}

void CommandList::Begin()
{
    VkCommandBufferBeginInfo info;
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.pNext = nullptr;
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    info.pInheritanceInfo = nullptr;

    VK_ASSERT(table_->vkBeginCommandBuffer(commandBuffer_, &info));
    DRE_DEBUG_ONLY(isOpened_ = true);
}

void CommandList::End()
{
    DRE_ASSERT(isOpened_, "Attempt to call End on commandlist that was not opened.");
    VK_ASSERT(table_->vkEndCommandBuffer(commandBuffer_));
}

void CommandList::Reset()
{
    table_->vkResetCommandPool(device_->Handle(), commandPool_, VK_FLAGS_NONE/*VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT*/);
    DRE_DEBUG_ONLY(isOpened_ = false);
}

void CommandList::SetFinishExecutionPoint(QueueExecutionPoint& point)
{
    executionFinishPoint_ = point;
}

void CommandList::WaitForPendingExecution()
{
    VkSemaphore timelineSemaphore = executionFinishPoint_.GetTimelineSemaphore();
    executionFinishPoint_.Wait();
}

///////////////////////////////////////////
Queue::Queue()
    : table_{ nullptr }
    , device_{ nullptr }
    , queue_{ VK_NULL_HANDLE }
    , queueFamily_{ INVALID_QUEUE }
    , queueIndex_{ INVALID_QUEUE }
    , timelineSemaphore_{ VK_NULL_HANDLE }
    , submitCounter_{ 0 }
{
}

Queue::Queue(ImportTable* table, LogicalDevice* device, std::uint32_t queueFamily, std::uint32_t queueIndex)
    : table_{ table }
    , device_{ device }
    , queue_{ VK_NULL_HANDLE }
    , queueFamily_{ queueFamily }
    , queueIndex_{ queueIndex }
    , timelineSemaphore_{ VK_NULL_HANDLE }
    , submitCounter_{ 0 }
{
    table_->vkGetDeviceQueue(device_->Handle(), queueFamily_, queueIndex, &queue_);
    assert(queue_ != VK_NULL_HANDLE && "Can't get device queue.");

    commandListPool_.Init(VKW::CONSTANTS::MAX_COMMANDLIST_PER_QUEUE, table, device, this);

    VkSemaphoreCreateInfo semaphoreInfo;
    VkSemaphoreTypeCreateInfo typeInfo;

    typeInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    typeInfo.pNext = nullptr;
    typeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    typeInfo.initialValue = 0;

    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreInfo.pNext = &typeInfo;
    semaphoreInfo.flags = VK_FLAGS_NONE;

    VK_ASSERT(table_->vkCreateSemaphore(device_->Handle(), &semaphoreInfo, nullptr, &timelineSemaphore_));
}

Queue::Queue(Queue&& rhs)
{
    operator=(std::move(rhs));
}

Queue& Queue::operator=(Queue&& rhs)
{
    DRE_SWAP_MEMBER(table_);
    DRE_SWAP_MEMBER(device_);
    DRE_SWAP_MEMBER(queue_);
    DRE_SWAP_MEMBER(queueFamily_);
    DRE_SWAP_MEMBER(queueIndex_);
    DRE_SWAP_MEMBER(timelineSemaphore_);
    DRE_SWAP_MEMBER(submitCounter_);
    DRE_SWAP_MEMBER(commandListPool_);

    return *this;
}

CommandList* Queue::GetFreeCommandList()
{
    CommandList* commandList = commandListPool_.AcquireObject();
    commandList->WaitForPendingExecution();
    commandList->Reset();
    commandList->Begin();
    return commandList;
}

QueueExecutionPoint Queue::ScheduleExecute(CommandList* commandList, QueueExecutionPoint const& waitPoint)
{
    return ScheduleExecute(commandList, 1, &waitPoint);
}

QueueExecutionPoint Queue::Execute(CommandList* commandList, QueueExecutionPoint const& waitPoint)
{
    return Execute(commandList, 1, &waitPoint);
}

QueueExecutionPoint Queue::ScheduleExecute(CommandList* commandList, std::uint8_t waitPointCount, QueueExecutionPoint const* waitPoints)
{
    std::uint64_t signalValue = submitCounter_++;
    QueueExecutionPoint executionPoint{ this, signalValue };

    PendingCommandList pendingCmdList;
    pendingCmdList.commandList_ = commandList;
    pendingCmdList.waitCount_ = waitPointCount;
    for (std::uint8_t i = 0; i < waitPointCount; i++)
    {
        pendingCmdList.waitPoints_[i] = waitPoints[i];
    }

    pendingCommandLists_.EmplaceBack(pendingCmdList);
    return executionPoint;
}

QueueExecutionPoint Queue::Execute(CommandList* commandList, std::uint8_t waitPointCount, QueueExecutionPoint const* waitPoints)
{
    ExecutePending();
    return ExecuteInternal(commandList, waitPointCount, waitPoints, nullptr);
}

void Queue::ExecutePending()
{
    for (std::uint32_t i = 0; i < pendingCommandLists_.Size(); i++)
    {
        PendingCommandList& pending = pendingCommandLists_[i];
        ExecuteInternal(pending.commandList_, pending.waitCount_, pending.waitPoints_, nullptr);
    }
    pendingCommandLists_.Clear();
}

QueueExecutionPoint Queue::ExecuteWaitSwapchain(CommandList* commandList, PresentationContext& presentationContext, std::uint8_t waitPointCount, QueueExecutionPoint const* waitPoints)
{
    ExecutePending();
    return ExecuteInternal(commandList, waitPointCount, waitPoints, &presentationContext);
}

QueueExecutionPoint Queue::ExecuteWaitSwapchain(CommandList* commandList, PresentationContext& presentationContext, QueueExecutionPoint const& waitPoint)
{
    return ExecuteWaitSwapchain(commandList, presentationContext, 1, &waitPoint);
}

QueueExecutionPoint Queue::ExecuteInternal(CommandList* commandList, std::uint8_t waitPointCount, QueueExecutionPoint const* waitExecutionPoints, PresentationContext* presentationContext)
{
    commandList->End();
    VkCommandBuffer vkCommandBuffer = *commandList;

    std::uint8_t const binary = presentationContext == nullptr ? 0 : 1;

    DRE_ASSERT(waitPointCount + binary <= WAIT_COUNT_MAX, "Can't wait for more that 5 semaphores");
    static VkPipelineStageFlags const waitStage[WAIT_COUNT_MAX] = {
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
    };

    static std::uint64_t semaphoreWaitValues[WAIT_COUNT_MAX];

    std::uint64_t const signalValues[2] = { ++submitCounter_, 0 };
    VkSemaphore signalSemaphores[2] = { timelineSemaphore_, binary ? presentationContext->GetRenderingCompleteSemaphore() : VK_NULL_HANDLE };

    VkTimelineSemaphoreSubmitInfo semaphoreInfo;
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
    semaphoreInfo.pNext = nullptr;
    semaphoreInfo.waitSemaphoreValueCount = waitPointCount + binary;
    semaphoreInfo.pWaitSemaphoreValues = semaphoreWaitValues;
    semaphoreInfo.signalSemaphoreValueCount = 1 + binary;
    semaphoreInfo.pSignalSemaphoreValues = signalValues;

    VkSemaphore waitSemaphores[WAIT_COUNT_MAX] = {
        VK_NULL_HANDLE,
        VK_NULL_HANDLE,
        VK_NULL_HANDLE,
        VK_NULL_HANDLE,
        VK_NULL_HANDLE
    };

    if (binary != 0)
    {
        semaphoreWaitValues[0] = 0;
        waitSemaphores[0] = presentationContext->GetSwapchainReleaseSemaphore();
    }
    for (std::uint8_t i = binary; i < waitPointCount + binary; i++)
    {
        semaphoreWaitValues[i] = waitExecutionPoints[i].GetPoint();
        waitSemaphores[i] = waitExecutionPoints[i].GetTimelineSemaphore();
    }

    VkSubmitInfo submitInfo;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = &semaphoreInfo;
    submitInfo.waitSemaphoreCount = waitPointCount;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkCommandBuffer;
    submitInfo.signalSemaphoreCount = 1 + binary;
    submitInfo.pSignalSemaphores = signalSemaphores;

    table_->vkQueueSubmit(queue_, 1, &submitInfo, VK_NULL_HANDLE);

    QueueExecutionPoint executionEndPoint{ this, signalValues[0] }; 
    commandList->SetFinishExecutionPoint(executionEndPoint);

    commandListPool_.ReturnObject(commandList);

    return executionEndPoint;
}

void Queue::ReturnCommandList(CommandList* commandList)
{
    commandList->End();
    commandListPool_.ReturnObject(commandList);
}

void Queue::WaitFor(QueueExecutionPoint const& point)
{
    WaitFor(1, &point);
}

void Queue::WaitFor(std::uint8_t pointCount, QueueExecutionPoint const* points)
{
    DRE_ASSERT(pointCount <= WAIT_COUNT_MAX, "Can't wait for more that 5 semaphores");
    
    VkSemaphore semaphores[WAIT_COUNT_MAX];
    std::uint64_t values[WAIT_COUNT_MAX];

    for (std::uint8_t i = 0; i < pointCount; i++)
    {
        semaphores[i] = points[i].GetTimelineSemaphore();
        values[i] = points[i].GetPoint();
    }

    VkSemaphoreWaitInfo waitInfo;
    waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
    waitInfo.pNext = nullptr;
    waitInfo.flags = VK_FLAGS_NONE;
    waitInfo.semaphoreCount = pointCount;
    waitInfo.pSemaphores = semaphores;
    waitInfo.pValues = values;

    table_->vkWaitSemaphores(device_->Handle(), &waitInfo, UINT64_MAX);
}

void Queue::WaitIdle()
{
    VK_ASSERT(table_->vkQueueWaitIdle(GetHardwareQueue()));
}

Queue::~Queue()
{
    if (timelineSemaphore_ != VK_NULL_HANDLE)
    {
        table_->vkDestroySemaphore(device_->Handle(), timelineSemaphore_, nullptr);
    }
}

}