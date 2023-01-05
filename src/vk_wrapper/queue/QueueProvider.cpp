#include <vk_interface\queue\QueueProvider.hpp>

#include <foundation\Common.hpp>
#include <vk_interface\Tools.hpp>

namespace VKW
{

QueueProvider::QueueProvider()
    : table_{ nullptr }
    , device_{ nullptr }
    , mainQueue_{}
{
}

QueueProvider::QueueProvider(ImportTable* table, LogicalDevice* device)
    : table_{ table }
    , device_{ device }
    , mainQueue_{ table, device, FindFamilyIndex(device_, DeviceQueueType::GRAPHICS_PRESENT, 1), 0 }
{
    
}

QueueProvider::QueueProvider(QueueProvider&& rhs)
    : table_{ nullptr }
    , device_{ nullptr }
    , mainQueue_{}
{
    operator=(std::move(rhs));
}

QueueProvider& QueueProvider::operator=(QueueProvider&& rhs)
{
    DRE_SWAP_MEMBER(table_);
    DRE_SWAP_MEMBER(device_);
    DRE_SWAP_MEMBER(mainQueue_);

    return *this;
}

QueueProvider::~QueueProvider()
{
    VK_ASSERT(table_->vkQueueWaitIdle(mainQueue_.GetHardwareQueue()));
}

Queue* QueueProvider::GetLoadingQueue()
{
    return &mainQueue_;
}

Queue* QueueProvider::GetMainQueue()
{
    return &mainQueue_;
}

Queue* QueueProvider::GetPresentationQueue()
{
    return &mainQueue_;
}

std::uint32_t QueueProvider::FindFamilyIndex(LogicalDevice const* device, DeviceQueueType type, std::uint32_t requiredCount)
{
    std::uint32_t constexpr INVALID_RESULT = std::numeric_limits<std::uint32_t>::max();
    std::uint32_t result = INVALID_RESULT;

    if (requiredCount == 0) {
        return result;
    }

    bool const presentRequired = type == DeviceQueueType::GRAPHICS_PRESENT;

    std::uint32_t const familyCount = device->QueueFamilyCount();
    for (std::uint32_t i = 0u; i < familyCount; ++i) {
        VKW::DeviceQueueFamilyInfo const& familyDesc = device->GetQueueFamily(i);

        //if presentation is required on this type of family, skip family in case it doesn't support presentation
        if (presentRequired && !familyDesc.presentationSupported_) {
            continue;
        }

        if (familyDesc.type_ == type && 
            familyDesc.count_ >= requiredCount) {
            result = familyDesc.familyIndex_;
            break;
        }
    }

    assert(result != INVALID_RESULT && "Couldn't find queue family index for WorkerGroup");

    return result;
}


}