#pragma once

#include <vector>
#include <string>

#include <foundation\class_features\NonCopyable.hpp>

#include <vk_wrapper\ImportTable.hpp>

namespace VKW
{

struct InstanceDesc
{
    ImportTable* table_;
    std::vector<std::string> requiredInstanceExtensions_;
    std::vector<std::string> requiredInstanceLayers_;
    bool debug_;
};

class Instance
    : public NonCopyable
{
public:
    Instance();
    Instance(InstanceDesc const& decs);

    Instance(Instance&& rhs);
    Instance& operator=(Instance&& rhs);
    
    VkInstance Handle() const;

    operator bool() const;

    ~Instance();

    static VKAPI_ATTR VkBool32 DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT              messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT*  pCallbackData,
        void* pUserData
    );


private:
    VKW::ImportTable* table_;
    VkInstance instance_;
    VkDebugUtilsMessengerEXT debugMessenger_;
};

}