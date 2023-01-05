#pragma once

#include <memory>
#include <utility>

#include <class_features\NonCopyable.hpp>

#include <vk_interface\LogicalDevice.hpp>
#include <vk_interface\queue\Queue.hpp>

namespace VKW
{

class Surface;

class QueueProvider
    : public NonCopyable
{
public:
    QueueProvider();
    QueueProvider(ImportTable* table, LogicalDevice* device);

    QueueProvider(QueueProvider&& rhs);
    QueueProvider& operator=(QueueProvider&& rhs);

    ~QueueProvider();

    Queue* GetLoadingQueue();
    Queue* GetMainQueue();
    Queue* GetPresentationQueue();

private:
    static std::uint32_t FindFamilyIndex(LogicalDevice const* device, DeviceQueueType type, std::uint32_t requiredCount);

private:
    ImportTable* table_;
    LogicalDevice* device_;
    
    Queue mainQueue_;
};

}