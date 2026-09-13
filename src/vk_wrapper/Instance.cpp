#include <vk_wrapper\Instance.hpp>

#include <foundation\Common.hpp>

#include <utility>
#include <algorithm>
#include <string>
#include <sstream>

#include <vk_wrapper/Tools.hpp>



namespace VKW
{

DRE::U32 Instance::s_BreakOnSeverity = DRE_U32_MAX;

Instance::Instance()
    : instance_{ VK_NULL_HANDLE }
    , table_{ nullptr }
    , debugMessenger_{ VK_NULL_HANDLE }
{

}

Instance::Instance(InstanceDesc const& desc)
    : instance_{ VK_NULL_HANDLE }
    , table_{ desc.table_ }
    , debugMessenger_{ VK_NULL_HANDLE }
{
    s_BreakOnSeverity = desc.validationBreakSeverity_;

    std::uint32_t layerPropertiesCount = 0;
    std::vector<VkLayerProperties> instanceLayerProperties;

    std::uint32_t extensionPropertiesCount;
    std::vector<VkExtensionProperties> extensionProperties;


    {
        VK_ASSERT(table_->vkEnumerateInstanceLayerProperties(&layerPropertiesCount, nullptr));
        instanceLayerProperties.resize(layerPropertiesCount);
        VK_ASSERT(table_->vkEnumerateInstanceLayerProperties(&layerPropertiesCount, instanceLayerProperties.data()));

        for (auto const& requiredLayer : desc.requiredInstanceLayers_) {
            auto const result = std::find_if(instanceLayerProperties.begin(), instanceLayerProperties.end(), [&requiredLayer](auto const& layer)
            {
                return requiredLayer == layer.layerName;
            });

            assert(result != instanceLayerProperties.end() && "Implementation does not support layer required by the application.");
        }
    }


    {
        VK_ASSERT(table_->vkEnumerateInstanceExtensionProperties(nullptr, &extensionPropertiesCount, nullptr));
        extensionProperties.resize(extensionPropertiesCount);
        VK_ASSERT(table_->vkEnumerateInstanceExtensionProperties(nullptr, &extensionPropertiesCount, extensionProperties.data()));

        for (auto const& requiredExtension : desc.requiredInstanceExtensions_) {
            auto const result = std::find_if(extensionProperties.begin(), extensionProperties.end(), [&requiredExtension](auto const& extensionProp)
            {
                return requiredExtension == extensionProp.extensionName;
            });

            assert(result != extensionProperties.end() && "Implementation does not support extension required by the application.");
        }
    }


    VkApplicationInfo applicationInfo;
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pNext = nullptr;
    applicationInfo.pApplicationName = "VulkanResearch";
    applicationInfo.pEngineName = "VulkanEngine";
    applicationInfo.apiVersion = VK_MAKE_VERSION(1, 3, 0);
    applicationInfo.applicationVersion = 0;
    applicationInfo.engineVersion = 0;


    std::vector<char const*> enabledLayers;
    std::vector<char const*> enabledExtensions;

    std::transform(
        desc.requiredInstanceLayers_.begin(), desc.requiredInstanceLayers_.end(), 
        std::back_inserter(enabledLayers), [](auto const& layer) {
        return layer.c_str();
    });

    std::transform(desc.requiredInstanceExtensions_.begin(), desc.requiredInstanceExtensions_.end(), 
        std::back_inserter(enabledExtensions), [](auto const& extension) {
        return extension.c_str();
    });

    VkInstanceCreateInfo instanceCreateInfo;
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext = nullptr;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.flags = 0;
    instanceCreateInfo.enabledLayerCount = static_cast<std::uint32_t>(enabledLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = enabledLayers.data();
    instanceCreateInfo.enabledExtensionCount = static_cast<std::uint32_t>(enabledExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();

    VK_ASSERT(table_->vkCreateInstance(&instanceCreateInfo, nullptr, &instance_));

    table_->GetInstanceProcAddresses(instance_);

#ifdef DRE_DEBUG
    // Debug callbacks setup
    if (desc.debug_) {
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;

        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.pNext = nullptr;
        debugCreateInfo.flags = VK_FLAGS_NONE;
        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        debugCreateInfo.pfnUserCallback = &Instance::DebugCallback;


        //VkDebugReportCallbackCreateInfoEXT debugCallbackCreateInfo;
        //debugCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        //debugCallbackCreateInfo.pNext = nullptr;
        //debugCallbackCreateInfo.pfnCallback = &Instance::DebugCallback;
        //debugCallbackCreateInfo.flags = 
        //    /*VK_DEBUG_REPORT_INFORMATION_BIT_EXT |*/
        //    VK_DEBUG_REPORT_WARNING_BIT_EXT |
        //    VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
        //    VK_DEBUG_REPORT_ERROR_BIT_EXT/* |
        //    VK_DEBUG_REPORT_DEBUG_BIT_EXT*/;
        //debugCallbackCreateInfo.pUserData = nullptr;

        VK_ASSERT(table_->vkCreateDebugUtilsMessengerEXT(instance_, &debugCreateInfo, nullptr, &debugMessenger_));
    }
#endif // DRE_DEBUG
}

Instance::Instance(Instance&& rhs)
    : instance_{ VK_NULL_HANDLE }
    , table_{ nullptr }
    , debugMessenger_{ VK_NULL_HANDLE }
{
    operator=(std::move(rhs));
}

Instance& Instance::operator=(Instance&& rhs)
{
    std::swap(rhs.instance_, instance_);
    std::swap(rhs.table_, table_);

    return *this;
}

Instance::operator bool() const
{
    return instance_ != VK_NULL_HANDLE;
}

VkInstance Instance::Handle() const
{
    return instance_;
}

Instance::~Instance()
{
#ifdef DRE_DEBUG
    if (debugMessenger_)
        table_->vkDestroyDebugUtilsMessengerEXT(instance_, debugMessenger_, nullptr);
#endif // DRE_DEBUG

    if (instance_)
        table_->vkDestroyInstance(instance_, nullptr);

    instance_ = VK_NULL_HANDLE;
}

VkBool32 Instance::DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT          messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT                 messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT*     pCallbackData,
    void* pUserData)
{
    std::stringstream output;

	output << "[";

    bool TriggerDebugBreak = false;

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        output << "VERBOSE|";
        if (s_BreakOnSeverity >= 3)
        {
            TriggerDebugBreak = true;
        }
    }

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        output << "INFO|";
        if (s_BreakOnSeverity >= 2)
        {
            TriggerDebugBreak = true;
        }
    }

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        output << "WARNING|";
        if (s_BreakOnSeverity >= 1)
        {
            TriggerDebugBreak = true;
        }
    }

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        output << "ERROR|";
        if (s_BreakOnSeverity >= 0)
        {
            TriggerDebugBreak = true;
        }
    }

    /////////////////////

    std::string typeString;
    if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
        output << "VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT";
    if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
        output << "VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT";
    if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
        output << "VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT";
    if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT)
        output << "VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT";

    output << ']' << std::endl << "[OBJECT MAPPING]:" << std::endl;

    for (std::uint32_t i = 0; i < pCallbackData->objectCount; i++)
    {
        output << "\thandle=" << pCallbackData->pObjects[i].objectHandle << " (";
        char const* name = pCallbackData->pObjects[i].pObjectName;
        output << (name == nullptr ? "" : name) << ")" << std::endl;
    }

    output << "MESSAGE: " << pCallbackData->pMessage << std::endl << std::endl;

    auto str = output.str();
    std::cerr << str;
    OutputDebugStringA(str.c_str());

#ifdef DRE_DEBUG
    if (TriggerDebugBreak)
        DebugBreak();
#endif

     return VK_FALSE;
}

}