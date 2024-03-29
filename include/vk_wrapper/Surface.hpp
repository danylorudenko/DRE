#pragma once

#include <vulkan\vulkan.h>
#include <vector>
#include <Windows.h>

#include <foundation\class_features\NonCopyable.hpp>

namespace VKW
{

class ImportTable;
class Instance;
class LogicalDevice;

struct SurfaceDesc
{
    ImportTable* table_;
    Instance* instance_;
    LogicalDevice* device_;

#ifdef _WIN32
    HINSTANCE hInstance_;
    HWND hwnd_;
#endif
};

class Surface
    : public NonCopyable
{
public:
    Surface();
    Surface(SurfaceDesc const& desc);

    Surface(Surface&& rhs);
    Surface& operator=(Surface&& rhs);
    
    operator bool() const;

    VkSurfaceKHR Handle() const;
    VkSurfaceCapabilitiesKHR const& SurfaceCapabilities() const;
    std::vector<VkPresentModeKHR> const& PresentModes() const;
    std::vector<VkSurfaceFormatKHR> const& SurfaceFormats() const;
    std::vector<std::uint32_t> const& SupportedQueueFamilies() const;


    ~Surface();

private:
    ImportTable* table_;
    Instance* instance_;
    LogicalDevice* device_;

    VkSurfaceKHR surface_;

    VkSurfaceCapabilitiesKHR surfaceCapabilities_;
    std::vector<VkPresentModeKHR> presentModes_;
    std::vector<VkSurfaceFormatKHR> surfaceFormats_;
    std::vector<std::uint32_t> supportedQueueFamilies_;

};

}