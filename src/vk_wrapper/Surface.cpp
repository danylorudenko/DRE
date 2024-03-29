#include <vk_wrapper\Surface.hpp>

#include <vk_wrapper\LogicalDevice.hpp>
#include <vk_wrapper\Tools.hpp>
#include <vk_wrapper\Instance.hpp>

namespace VKW
{

Surface::Surface()
    : table_{ nullptr }
    , instance_{ nullptr }
    , device_{ nullptr }
    , surface_{ VK_NULL_HANDLE }
{

}

Surface::Surface(SurfaceDesc const& desc)
    : table_{ desc.table_ }
    , instance_{ desc.instance_ }
    , device_{ desc.device_ }
    , surface_{ VK_NULL_HANDLE }
{
    VkSurfaceKHR surface = VK_NULL_HANDLE;

#ifdef _WIN32
    VkWin32SurfaceCreateInfoKHR sInfo;
    sInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    sInfo.pNext = nullptr;
    sInfo.flags = VK_FLAGS_NONE;
    sInfo.hinstance = desc.hInstance_;
    sInfo.hwnd = desc.hwnd_;
    
    VK_ASSERT(table_->vkCreateWin32SurfaceKHR(instance_->Handle(), &sInfo, nullptr, &surface));
    surface_ = surface;
#endif

    if (surface) {
        VkPhysicalDevice const phDevice = device_->PhysicalDeviceHandle();

        {
            VK_ASSERT(table_->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phDevice, surface_, &surfaceCapabilities_));
        }

        {
            std::uint32_t modesCount = 0;
            VK_ASSERT(table_->vkGetPhysicalDeviceSurfacePresentModesKHR(phDevice, surface_, &modesCount, nullptr));
            presentModes_.resize(modesCount);
            VK_ASSERT(table_->vkGetPhysicalDeviceSurfacePresentModesKHR(phDevice, surface_, &modesCount, presentModes_.data()));

        }

        {
            std::uint32_t formatsCount = 0;
            VK_ASSERT(table_->vkGetPhysicalDeviceSurfaceFormatsKHR(phDevice, surface_, &formatsCount, nullptr));
            surfaceFormats_.resize(formatsCount);
            VK_ASSERT(table_->vkGetPhysicalDeviceSurfaceFormatsKHR(phDevice, surface_, &formatsCount, surfaceFormats_.data()));
        }

        {
            supportedQueueFamilies_.clear();
            
            auto const queueFamilyCount = device_->QueueFamilyCount();
            for (auto i = 0u; i < queueFamilyCount; ++i) {
                auto const familyIndex = device_->GetQueueFamily(i).familyIndex_;

                VkBool32 familySupported = VK_FALSE;
                VK_ASSERT(table_->vkGetPhysicalDeviceSurfaceSupportKHR(phDevice, familyIndex, surface_, &familySupported));
                
                if (familySupported == VK_TRUE) {
                    supportedQueueFamilies_.push_back(familyIndex);
                }
            }
        }

        
    }
}

Surface::Surface(Surface&& rhs)
    : table_{ nullptr }
    , instance_{ nullptr }
    , device_{ nullptr }
    , surface_{ VK_NULL_HANDLE }
{
    operator=(std::move(rhs));
}

Surface& Surface::operator=(Surface&& rhs)
{
    std::swap(table_, rhs.table_);
    std::swap(instance_, rhs.instance_);
    std::swap(device_, rhs.device_);

    std::swap(surface_, rhs.surface_);
    std::swap(surfaceCapabilities_, rhs.surfaceCapabilities_);
    std::swap(presentModes_, rhs.presentModes_);
    std::swap(surfaceFormats_, rhs.surfaceFormats_);

    return *this;
}

Surface::operator bool() const
{
    return surface_ != VK_NULL_HANDLE;
}

VkSurfaceKHR Surface::Handle() const
{
    return surface_;
}

VkSurfaceCapabilitiesKHR const& Surface::SurfaceCapabilities() const
{
    return surfaceCapabilities_;
}

std::vector<VkPresentModeKHR> const& Surface::PresentModes() const
{
    return presentModes_;
}

std::vector<VkSurfaceFormatKHR> const& Surface::SurfaceFormats() const
{
    return surfaceFormats_;
}

std::vector<std::uint32_t> const& Surface::SupportedQueueFamilies() const
{
    return supportedQueueFamilies_;
}

Surface::~Surface()
{
    if (surface_ != VK_NULL_HANDLE) {
        table_->vkDestroySurfaceKHR(instance_->Handle(), surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }
}

}