#pragma once

#include <vulkan\vulkan.h>

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\Container\ObjectPoolQueue.hpp>
#include <foundation\Container\InplaceVector.hpp>

#include <vk_wrapper\Constant.hpp>
#include <vk_wrapper\LogicalDevice.hpp>
#include <vk_wrapper\swapchain\PresentationController.hpp>

namespace VKW
{

class Queue;

/////////////////////////////////////
class QueueExecutionPoint
{
public:
    QueueExecutionPoint();
    QueueExecutionPoint(Queue* parentQueue, std::uint64_t point);
    inline std::uint64_t GetPoint() const { return point_; }
    inline Queue*        GetQueue() const { return parentQueue_; }
    inline VkSemaphore   GetTimelineSemaphore() const;

    void Wait();

private:
    std::uint64_t point_;
    Queue* parentQueue_;
};

/////////////////////////////////////
class CommandList 
    : public NonCopyable
{
public:
    CommandList(ImportTable* table, LogicalDevice* device, Queue* parentQueue);

    CommandList(CommandList&& rhs);
    CommandList& operator=(CommandList&& rhs);

    ~CommandList();

    inline operator VkCommandBuffer() const { return commandBuffer_; }
    Queue* GetQueue() const { return parentQueue_; }

    void Begin();
    void End();
    void Reset();
    void SetFinishExecutionPoint(QueueExecutionPoint& point);
    void WaitForPendingExecution();

private:
    ImportTable* table_;
    LogicalDevice* device_;

    VkCommandBuffer commandBuffer_;
    VkCommandPool   commandPool_;
    
    QueueExecutionPoint executionFinishPoint_;

    Queue*    parentQueue_;

    DRE_DEBUG_ONLY(bool isOpened_;)
};

/////////////////////////////////////
class Queue
    : public NonCopyable
{
public:
    static constexpr std::uint8_t WAIT_COUNT_MAX = 5;

    Queue();
    Queue(ImportTable* table, LogicalDevice* device, std::uint32_t queueFamilyIndex, std::uint32_t queueIndex);

    Queue(Queue&& rhs);
    Queue& operator=(Queue&& rhs);

    ~Queue();

    inline VkQueue          GetHardwareQueue() const { return queue_; }
    inline std::uint32_t    GetQueueFamily() const { return queueFamily_; }
    inline VkSemaphore      GetTimelineSemaphore() const { return timelineSemaphore_; }

    CommandList*        GetFreeCommandList();

    QueueExecutionPoint ScheduleExecute(CommandList* commandList, QueueExecutionPoint const& waitExecutionPoint);
    QueueExecutionPoint ScheduleExecute(CommandList* commandList, std::uint8_t waitPointCount = 0, QueueExecutionPoint const* waitExecutionPoints = nullptr);

    QueueExecutionPoint Execute(CommandList* commandList, QueueExecutionPoint const& waitExecutionPoint);
    QueueExecutionPoint Execute(CommandList* commandList, std::uint8_t waitPointCount = 0, QueueExecutionPoint const* waitExecutionPoints = nullptr);

    QueueExecutionPoint ExecuteWaitSwapchain(CommandList* commandList, PresentationContext& presentationContext, std::uint8_t waitPointCount = 0, QueueExecutionPoint const* waitPoints = nullptr);
    QueueExecutionPoint ExecuteWaitSwapchain(CommandList* commandList, PresentationContext& presentationContext, QueueExecutionPoint const& waitPoint);

    void                ReturnCommandList(CommandList* commandList);

    void                ExecutePending();

    void                WaitFor(QueueExecutionPoint const& point);
    void                WaitFor(std::uint8_t waitPointCount, QueueExecutionPoint const* point);

    void                WaitIdle();

private:
    QueueExecutionPoint ExecuteInternal(CommandList* commandList, std::uint8_t waitPointCount, QueueExecutionPoint const* waitExecutionPoints, PresentationContext* presentationContext);

private:
    ImportTable* table_;
    LogicalDevice* device_;
    
    VkQueue queue_;
    std::uint32_t queueFamily_;
    std::uint32_t queueIndex_;

    VkSemaphore     timelineSemaphore_;
    std::uint64_t   submitCounter_;

    DRE::ObjectPoolQueue<CommandList>        commandListPool_;


    struct PendingCommandList
    {
        CommandList* commandList_;
        std::uint8_t waitCount_;
        QueueExecutionPoint waitPoints_[WAIT_COUNT_MAX];
    };
    DRE::InplaceVector<PendingCommandList, VKW::CONSTANTS::MAX_COMMANDLIST_PER_QUEUE> pendingCommandLists_;

};

}