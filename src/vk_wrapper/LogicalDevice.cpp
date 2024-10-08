#include <vk_wrapper/LogicalDevice.hpp>

#include <vk_wrapper/Tools.hpp>

#include <iomanip>
#include <algorithm>
#include <memory>
#include <limits>

namespace VKW
{

LogicalDevice::LogicalDevice()
    : device_{ VK_NULL_HANDLE }
    , table_{ nullptr }
    , physicalDevice_{ VK_NULL_HANDLE }
    , physicalDeviceProperties_{}
{
}

LogicalDevice::LogicalDevice(DeviceDesc const& desc)
    : device_{ VK_NULL_HANDLE }
    , table_{ desc.table_ }
    , physicalDevice_{ VK_NULL_HANDLE }
    , physicalDeviceProperties_{}
{
    std::uint32_t physicalDeviceCount = 0;
    std::vector<VkPhysicalDevice> physicalDevices;
    {
        VK_ASSERT(table_->vkEnumeratePhysicalDevices(desc.instance_->Handle(), &physicalDeviceCount, nullptr));
        physicalDevices.resize(physicalDeviceCount);
        VK_ASSERT(table_->vkEnumeratePhysicalDevices(desc.instance_->Handle(), &physicalDeviceCount, physicalDevices.data()));
    }
    

    // Pick physical device
    {
        if (physicalDeviceCount == 0) {
            std::cerr << "FATAL: Error initializing VKW::Device (Vulkan instance couldn't find any physical devices in the system)" << std::endl;
            assert(physicalDeviceCount != 0);
        }


        auto validPhysicalDevices = std::vector<VkPhysicalDevice>{};
        auto deviceProperties = std::make_unique<PhysicalDeviceProperties>();

        for (auto i = 0u; i < physicalDeviceCount; ++i) {
            
            RequestDeviceProperties(physicalDevices[i], *deviceProperties);

            bool deviceValid = IsPhysicalDeviceValid(*deviceProperties, desc.requiredExtensions_);
            if (deviceValid) {
                validPhysicalDevices.emplace_back(physicalDevices[i]);
            }

            PrintPhysicalDeviceData(*deviceProperties);

            *deviceProperties = PhysicalDeviceProperties{};
        }

        if (validPhysicalDevices.size() == 1) {
            physicalDevice_ = validPhysicalDevices[0];

        }
        else if (validPhysicalDevices.size() > 1) {
            for (auto i = 0u; i < validPhysicalDevices.size(); ++i) {

                RequestDeviceProperties(validPhysicalDevices[i], *deviceProperties);
                bool const isGPU = deviceProperties->properties2.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
                *deviceProperties = PhysicalDeviceProperties{};
                
                if (isGPU) {
                    physicalDevice_ = validPhysicalDevices[i];
                    break;
                }
            }

            // if no discrete GPUs, choose any first
            if (physicalDevice_ == VK_NULL_HANDLE) {
                physicalDevice_ = validPhysicalDevices[0];
            }
        }



        if (physicalDevice_ != VK_NULL_HANDLE) {
            RequestDeviceProperties(physicalDevice_, physicalDeviceProperties_);
            std::cout << "CHOSEN DEVICE: " << physicalDeviceProperties_.properties2.properties.deviceName << std::endl;
        }
        else {
            std::cout << "FATAL: Error initializing VKW::Device (cannot find valid VkPhysicalDevice)" << std::endl;
            assert(false && "FATAL: Error initializing VKW::Device (cannot find valid VkPhysicalDevice)");
        }
    }


    // Create logical device
    {
        auto constexpr QUEUE_TYPE_COUNT = 3;
        VkFlags constexpr QUEUE_TYPE_FLAGS[QUEUE_TYPE_COUNT] = {
            VK_QUEUE_GRAPHICS_BIT,
            VK_QUEUE_COMPUTE_BIT,
            VK_QUEUE_TRANSFER_BIT
        };

        std::uint32_t const QUEUE_COUNTS[QUEUE_TYPE_COUNT] = {
            desc.graphicsPresentQueueCount_,
            desc.computeQueueCount_,
            desc.transferQueueCount_
        };
        
        auto constexpr INVALID_QUEUE_INDEX = std::numeric_limits<std::uint32_t>::max();
        auto const& queueFamilyProperties = physicalDeviceProperties_.queueFamilyProperties;
        auto const& presentationFamilies = physicalDeviceProperties_.presentationFamilies;

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfoVec;
        std::vector<float*> queuePrioritiesVec;
        
        for(auto i = 0u; i < QUEUE_TYPE_COUNT; ++i) {
            if (QUEUE_COUNTS[i] == 0)
                continue;

            std::uint32_t chosenQueueFamily = INVALID_QUEUE_INDEX;
            for (auto j = 0u; j < queueFamilyProperties.size(); ++j) {
                bool const queueTypeGraphics = queueFamilyProperties[j].queueFlags & QUEUE_TYPE_FLAGS[0];

                bool const queueTypeSupported = queueFamilyProperties[j].queueFlags & QUEUE_TYPE_FLAGS[i];
                bool const queueCountSupported = queueFamilyProperties[j].queueCount >= QUEUE_COUNTS[i];
                bool const queuePresentSupported = std::find(presentationFamilies.cbegin(), presentationFamilies.cend(), j) != presentationFamilies.cend() ? true : false;

                if(queueTypeGraphics && !queuePresentSupported)
                    continue;


                if (queueTypeSupported && queueCountSupported) {
                    VkDeviceQueueCreateInfo queueCreateInfo;
                    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                    queueCreateInfo.pNext = nullptr;
                    queueCreateInfo.queueFamilyIndex = chosenQueueFamily = j;
                    queueCreateInfo.queueCount = QUEUE_COUNTS[i];
                    queueCreateInfo.flags = VK_FLAGS_NONE;
                    float* queuePriorities = new float[QUEUE_COUNTS[i]];
                    for (auto k = 0u; k < QUEUE_COUNTS[i]; ++k)
                        queuePriorities[k] = 1.0f;
                    queueCreateInfo.pQueuePriorities = queuePriorities;

                    queuePrioritiesVec.emplace_back(queuePriorities);
                    queueCreateInfoVec.emplace_back(queueCreateInfo);

                    // Save queues info for own usage
                    DeviceQueueFamilyInfo queueInfo;
                    queueInfo.familyIndex_ = chosenQueueFamily;
                    queueInfo.count_ = QUEUE_COUNTS[i];
                    queueInfo.presentationSupported_ = queuePresentSupported;
                    if (QUEUE_TYPE_FLAGS[i] & VK_QUEUE_GRAPHICS_BIT) {
                        queueInfo.type_ = DeviceQueueType::GRAPHICS_PRESENT;
                    }
                    else if (QUEUE_TYPE_FLAGS[i] & VK_QUEUE_COMPUTE_BIT) {
                        queueInfo.type_ = DeviceQueueType::COMPUTE;
                    }
                    else if (QUEUE_TYPE_FLAGS[i] & VK_QUEUE_TRANSFER_BIT) {
                        queueInfo.type_ = DeviceQueueType::TRANSFER;
                    }
                    queueInfo_.emplace_back(queueInfo);

                    break;
                }
            }

            if (QUEUE_COUNTS[i] > 0) {
                assert(chosenQueueFamily != INVALID_QUEUE_INDEX && "Couldn't create all required queues");
            }
        }

        std::vector<char const*> requiredExtensionsC_str{};
        std::transform(
            desc.requiredExtensions_.begin(), desc.requiredExtensions_.end(), 
            std::back_inserter(requiredExtensionsC_str), 
            [](auto const& string){ return string.c_str(); });

        DisableHeavyPhysicalDeviceFeatures();

        VkDeviceCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pNext = &physicalDeviceProperties_.features2;
        createInfo.flags = VK_FLAGS_NONE;
        createInfo.pEnabledFeatures = nullptr;
        createInfo.queueCreateInfoCount = static_cast<std::uint32_t>(queueCreateInfoVec.size());
        createInfo.pQueueCreateInfos = queueCreateInfoVec.data();
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;
        createInfo.enabledExtensionCount = static_cast<std::uint32_t>(requiredExtensionsC_str.size());
        createInfo.ppEnabledExtensionNames = requiredExtensionsC_str.data();


        VK_ASSERT(table_->vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_));

        for (auto const arr : queuePrioritiesVec) {
            delete[] arr;
        }

        table_->GetDeviceProcAddresses(device_);
    }

}

LogicalDevice::LogicalDevice(LogicalDevice&& rhs)
    : device_{ VK_NULL_HANDLE }
    , table_{ nullptr }
    , physicalDevice_{ VK_NULL_HANDLE }
    , physicalDeviceProperties_{}
{
    operator=(std::move(rhs));
}

LogicalDevice& LogicalDevice::operator=(LogicalDevice&& rhs)
{
    std::swap(device_, rhs.device_);
    std::swap(table_, rhs.table_);
    std::swap(physicalDevice_, rhs.physicalDevice_);
    std::swap(physicalDeviceProperties_, rhs.physicalDeviceProperties_);
    std::swap(queueInfo_, rhs.queueInfo_);

    return *this;
}

VkDevice LogicalDevice::Handle() const
{
    return device_;
}

VkPhysicalDevice LogicalDevice::PhysicalDeviceHandle() const
{
    return physicalDevice_;
}

LogicalDevice::operator bool() const
{
    return device_ != VK_NULL_HANDLE;
}

bool LogicalDevice::IsAPI13Supported() const
{
    return IsAPI13SupportedByPhysicalDevice(physicalDeviceProperties_.properties2.properties);
}

void LogicalDevice::PrintPhysicalDeviceFormatProperties(VkFormat format)
{
    VkFormatProperties fProps;
    table_->vkGetPhysicalDeviceFormatProperties(physicalDevice_, format, &fProps);
    
    bool r1 = fProps.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
    bool r2 = fProps.linearTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
    bool r3 = fProps.linearTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT;
    bool r4 = fProps.linearTilingFeatures & VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT;
    bool r5 = fProps.linearTilingFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT;
    bool r6 = fProps.linearTilingFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT;
    bool r7 = fProps.linearTilingFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT;
    bool r8 = fProps.linearTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
    bool r9 = fProps.linearTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT;
    bool r10 = fProps.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    bool r12 = fProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT;
    bool r13 = fProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT;
    bool r14 = fProps.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
    bool r15 = fProps.linearTilingFeatures & VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
    bool r16 = fProps.linearTilingFeatures & VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
    bool r18 = fProps.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_IMG;
    bool r19 = fProps.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_MINMAX_BIT_EXT;
    bool r20 = false;

    std::cout << "Not implemented!" << std::endl;
}

VKW::LogicalDevice::PhysicalDeviceProperties const& LogicalDevice::Properties() const
{
    return physicalDeviceProperties_;
}

std::uint32_t LogicalDevice::QueueFamilyCount() const
{
    return static_cast<std::uint32_t>(queueInfo_.size());
}

VKW::DeviceQueueFamilyInfo const& LogicalDevice::GetQueueFamily(std::uint32_t index) const
{
    return queueInfo_[index];
}

LogicalDevice::~LogicalDevice()
{
    if (device_) {
        VK_ASSERT(table_->vkDeviceWaitIdle(device_));
        table_->vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }
}

bool LogicalDevice::IsPhysicalDeviceValid(
    VKW::LogicalDevice::PhysicalDeviceProperties const& deviceProperties,
    std::vector<std::string> const& requiredExtensions)
{
#ifndef DRE_COMPILE_FOR_RENDERDOC
    VkPhysicalDeviceExtendedDynamicState3FeaturesEXT const& extendedDynamicFeatures = *((VkPhysicalDeviceExtendedDynamicState3FeaturesEXT const*)deviceProperties.features2.pNext);
    VkPhysicalDeviceVulkan12Features const& vulkan12Features = *((VkPhysicalDeviceVulkan12Features const*)extendedDynamicFeatures.pNext);
#else
    VkPhysicalDeviceVulkan12Features const& vulkan12Features = *((VkPhysicalDeviceVulkan12Features const*)deviceProperties.features2.pNext);
#endif
    VkPhysicalDeviceVulkan13Features const& vulkan13Features = *((VkPhysicalDeviceVulkan13Features const*)vulkan12Features.pNext);

    bool supportsGraphics = false;
    bool supportsExtensions = true;
    bool supportsSurface = false;
    bool supports13 = IsAPI13SupportedByPhysicalDevice(deviceProperties.properties2.properties);
    bool supportsFeatures = vulkan12Features.descriptorIndexing && vulkan13Features.dynamicRendering;

    auto const& queueFamilyProperties = deviceProperties.queueFamilyProperties;
    for (auto i = 0u; i < queueFamilyProperties.size(); ++i) {
        auto& queueProps = queueFamilyProperties[i];
        if (queueProps.queueFlags & VK_QUEUE_GRAPHICS_BIT &&
            queueProps.queueCount > 0)
        {
            supportsGraphics = true;
            break;
        }
    }

    auto const& supportedExtensions = deviceProperties.extensionProperties;
    for (auto i = 0u; i < requiredExtensions.size(); ++i) {
        const auto& extName = requiredExtensions[i];
        auto result = std::find_if(supportedExtensions.cbegin(), supportedExtensions.cend(),
        [&extName](auto const& extensionProperties) {
            return extName == extensionProperties.extensionName;
        });

        if (result == supportedExtensions.cend()) {
            supportsExtensions = false;
        }
    }

    if (deviceProperties.presentationFamilies.size() > 0) {
        supportsSurface = true;
    }

    std::cerr << "SUPPORT REPORT: ";
        std::cerr << supportsGraphics << ' ' << supportsExtensions << ' ' << supportsSurface << ' ' << supports13 << ' ' << supportsFeatures << std::endl;

    return supportsGraphics && supportsExtensions && supportsSurface && supports13 && supportsFeatures;
}

void LogicalDevice::DisableHeavyPhysicalDeviceFeatures()
{
#ifndef DRE_COMPILE_FOR_RENDERDOC
    VkPhysicalDeviceExtendedDynamicState3FeaturesEXT& dynamicStateFeatures = *((VkPhysicalDeviceExtendedDynamicState3FeaturesEXT*)physicalDeviceProperties_.features2.pNext);
#endif
    VkPhysicalDeviceFeatures& features = physicalDeviceProperties_.features2.features;

    RELEASE_ONLY(features.robustBufferAccess        = VK_FALSE);

    features.geometryShader                         = VK_FALSE;
    features.tessellationShader                     = VK_FALSE;
    features.textureCompressionETC2                 = VK_FALSE;
    features.textureCompressionASTC_LDR             = VK_FALSE;
    features.shaderTessellationAndGeometryPointSize = VK_FALSE;
    features.sparseBinding                          = VK_FALSE;
    features.sparseResidencyBuffer                  = VK_FALSE;
    features.sparseResidencyImage2D                 = VK_FALSE;
    features.sparseResidencyImage3D                 = VK_FALSE;
    features.sparseResidency2Samples                = VK_FALSE;
    features.sparseResidency4Samples                = VK_FALSE;
    features.sparseResidency8Samples                = VK_FALSE;
    features.sparseResidency16Samples               = VK_FALSE;
    features.sparseResidencyAliased                 = VK_FALSE;

#ifndef DRE_COMPILE_FOR_RENDERDOC
    dynamicStateFeatures.extendedDynamicState3TessellationDomainOrigin           = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3DepthClampEnable                   = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3PolygonMode                        = VK_TRUE; /////// VK_TRUE
    dynamicStateFeatures.extendedDynamicState3RasterizationSamples               = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3SampleMask                         = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3AlphaToCoverageEnable              = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3AlphaToOneEnable                   = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3LogicOpEnable                      = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3ColorBlendEnable                   = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3ColorBlendEquation                 = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3ColorWriteMask                     = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3RasterizationStream                = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3ConservativeRasterizationMode      = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3ExtraPrimitiveOverestimationSize   = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3DepthClipEnable                    = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3SampleLocationsEnable              = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3ColorBlendAdvanced                 = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3ProvokingVertexMode                = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3LineRasterizationMode              = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3LineStippleEnable                  = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3DepthClipNegativeOneToOne          = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3ViewportWScalingEnable             = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3ViewportSwizzle                    = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3CoverageToColorEnable              = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3CoverageToColorLocation            = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3CoverageModulationMode             = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3CoverageModulationTableEnable      = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3CoverageModulationTable            = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3CoverageReductionMode              = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3RepresentativeFragmentTestEnable   = VK_FALSE;
    dynamicStateFeatures.extendedDynamicState3ShadingRateImageEnable             = VK_FALSE;
#endif

}

void LogicalDevice::RequestDeviceProperties(
    VkPhysicalDevice targetDevice,
    VKW::LogicalDevice::PhysicalDeviceProperties& deviceProperties)
{
    ToolSetMemZero(deviceProperties.properties2);
    ToolSetMemZero(deviceProperties.memoryProperties2);
    ToolSetMemZero(deviceProperties.memoryBudgetProperties);
    ToolSetMemZero(deviceProperties.vulkan12Properties);
    ToolSetMemZero(deviceProperties.descriptorIndexingProperties);
    ToolSetMemZero(deviceProperties.features2);
    ToolSetMemZero(deviceProperties.vulkan12Features);
    deviceProperties.queueFamilyProperties.clear();
    deviceProperties.extensionProperties.clear();


    deviceProperties.properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProperties.properties2.pNext = &deviceProperties.vulkan12Properties;

    deviceProperties.vulkan12Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
    deviceProperties.vulkan12Properties.pNext = &deviceProperties.descriptorIndexingProperties;

    deviceProperties.descriptorIndexingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES;
    deviceProperties.descriptorIndexingProperties.pNext = nullptr;

    table_->vkGetPhysicalDeviceProperties2(targetDevice, &deviceProperties.properties2);


    deviceProperties.memoryProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
    deviceProperties.memoryProperties2.pNext = &deviceProperties.memoryBudgetProperties;

    deviceProperties.memoryBudgetProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;
    deviceProperties.memoryBudgetProperties.pNext = nullptr;

    table_->vkGetPhysicalDeviceMemoryProperties2(targetDevice, &deviceProperties.memoryProperties2);

#ifndef DRE_COMPILE_FOR_RENDERDOC
    deviceProperties.features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceProperties.features2.pNext = &deviceProperties.extendedDynamicState3FeaturesEXT;

    deviceProperties.extendedDynamicState3FeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;
    deviceProperties.extendedDynamicState3FeaturesEXT.pNext = &deviceProperties.vulkan12Features;
#else
    deviceProperties.features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceProperties.features2.pNext = &deviceProperties.vulkan12Features;
#endif

    deviceProperties.vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    deviceProperties.vulkan12Features.pNext = &deviceProperties.vulkan13Features;

    deviceProperties.vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    deviceProperties.vulkan13Features.pNext = &deviceProperties.accelerationStructureFeatures;

    deviceProperties.accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    deviceProperties.accelerationStructureFeatures.pNext = &deviceProperties.rayTracingPipelineFeatures;

    deviceProperties.rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    deviceProperties.rayTracingPipelineFeatures.pNext = nullptr;

    table_->vkGetPhysicalDeviceFeatures2(targetDevice, &deviceProperties.features2);


    std::uint32_t queuePropsCount = 0u;
    table_->vkGetPhysicalDeviceQueueFamilyProperties(targetDevice, &queuePropsCount, nullptr);
    deviceProperties.queueFamilyProperties.resize(queuePropsCount);
    table_->vkGetPhysicalDeviceQueueFamilyProperties(targetDevice, &queuePropsCount, deviceProperties.queueFamilyProperties.data());

#ifdef _WIN32
    deviceProperties.presentationFamilies.clear();
    for (std::uint32_t i = 0u; i < queuePropsCount; ++i) {
        VkBool32 presentationSupport = table_->vkGetPhysicalDeviceWin32PresentationSupportKHR(targetDevice, i);
        if (presentationSupport == VK_TRUE) {
            deviceProperties.presentationFamilies.push_back(i);
        }
    }
#endif

    std::uint32_t extensionPropsCount = 0u;
    VK_ASSERT(table_->vkEnumerateDeviceExtensionProperties(targetDevice, nullptr, &extensionPropsCount, nullptr));
    deviceProperties.extensionProperties.resize(extensionPropsCount);
    VK_ASSERT(table_->vkEnumerateDeviceExtensionProperties(targetDevice, nullptr, &extensionPropsCount, deviceProperties.extensionProperties.data()));
}

bool LogicalDevice::IsAPI13SupportedByPhysicalDevice(VkPhysicalDeviceProperties const& physicalDeviceProperties)
{
    std::uint32_t const physicalDeviceApiVersion_MAJOR = VK_VERSION_MAJOR(physicalDeviceProperties.apiVersion);
    std::uint32_t const physicalDeviceApiVersion_MINOR = VK_VERSION_MINOR(physicalDeviceProperties.apiVersion);

    if (physicalDeviceApiVersion_MAJOR >= 1 && physicalDeviceApiVersion_MINOR >= 3)
        return true;
    else
        return false;
}

void LogicalDevice::PrintPhysicalDeviceData(VKW::LogicalDevice::PhysicalDeviceProperties const& deviceProperties)
{
    auto const& properties = deviceProperties.properties2.properties;
    char const* deviceTypeStr = nullptr;

    switch (properties.deviceType)
    {
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
        deviceTypeStr = "VK_PHYSICAL_DEVICE_TYPE_OTHER";
        break;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        deviceTypeStr = "VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU";
        break;
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        deviceTypeStr = "VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU";
        break;
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        deviceTypeStr = "VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU";
        break;
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
        deviceTypeStr = "VK_PHYSICAL_DEVICE_TYPE_CPU";
        break;
    default:
        deviceTypeStr = "Unknown";
    }

    std::cout << "PHYSICAL DEVICE NAME: " << properties.deviceName << std::endl << std::endl;
    std::cout << "\t" << "Vendor ID: " << properties.vendorID << std::endl;
    std::cout << "\t" << "Device ID: " << properties.deviceID << std::endl;
    std::cout << "\t" << "Type: " << deviceTypeStr << std::endl;
    std::cout << "\t" << "Driver version: " << properties.driverVersion << std::endl;
    std::cout << "\t" << "API Version: " << VK_VERSION_MAJOR(properties.apiVersion) << '.' << VK_VERSION_MINOR(properties.apiVersion) << '.' << VK_VERSION_PATCH(properties.apiVersion) << std::endl;

    std::cout << "\t" << "Device limits: " << std::endl;
    std::cout << "\t\t" << "maxImageDimension1D:                              " << properties.limits.maxImageDimension1D << std::endl;
    std::cout << "\t\t" << "maxImageDimension2D:                              " << properties.limits.maxImageDimension2D << std::endl;
    std::cout << "\t\t" << "maxImageDimension3D:                              " << properties.limits.maxImageDimension3D << std::endl;
    std::cout << "\t\t" << "maxImageDimensionCube:                            " << properties.limits.maxImageArrayLayers << std::endl;
    std::cout << "\t\t" << "maxImageArrayLayers:                              " << properties.limits.maxImageDimensionCube << std::endl;
    std::cout << "\t\t" << "maxTexelBufferElements:                           " << properties.limits.maxTexelBufferElements << std::endl;
    std::cout << "\t\t" << "maxUniformBufferRange:                            " << properties.limits.maxUniformBufferRange << std::endl;
    std::cout << "\t\t" << "maxStorageBufferRange:                            " << properties.limits.maxStorageBufferRange << std::endl;
    std::cout << "\t\t" << "maxPushConstantsSize:                             " << properties.limits.maxPushConstantsSize << std::endl;
    std::cout << "\t\t" << "maxMemoryAllocationCount:                         " << properties.limits.maxMemoryAllocationCount << std::endl;
    std::cout << "\t\t" << "maxSamplerAllocationCount:                        " << properties.limits.maxSamplerAllocationCount << std::endl;
    std::cout << "\t\t" << "bufferImageGranularity:                           " << properties.limits.bufferImageGranularity << std::endl;
    std::cout << "\t\t" << "sparseAddressSpaceSize:                           " << properties.limits.sparseAddressSpaceSize << std::endl;
    std::cout << "\t\t" << "maxBoundDescriptorSets:                           " << properties.limits.maxBoundDescriptorSets << std::endl;
    std::cout << "\t\t" << "maxPerStageDescriptorSamplers:                    " << properties.limits.maxPerStageDescriptorSamplers << std::endl;
    std::cout << "\t\t" << "maxPerStageDescriptorUniformBuffers:              " << properties.limits.maxPerStageDescriptorUniformBuffers << std::endl;
    std::cout << "\t\t" << "maxPerStageDescriptorStorageBuffers:              " << properties.limits.maxPerStageDescriptorStorageBuffers << std::endl;
    std::cout << "\t\t" << "maxPerStageDescriptorSampledImages:               " << properties.limits.maxPerStageDescriptorSampledImages << std::endl;
    std::cout << "\t\t" << "maxPerStageDescriptorStorageImages:               " << properties.limits.maxPerStageDescriptorStorageImages << std::endl;
    std::cout << "\t\t" << "maxPerStageDescriptorInputAttachments:            " << properties.limits.maxPerStageDescriptorInputAttachments << std::endl;
    std::cout << "\t\t" << "maxPerStageResources:                             " << properties.limits.maxPerStageResources << std::endl;
    std::cout << "\t\t" << "maxDescriptorSetSamplers:                         " << properties.limits.maxDescriptorSetSamplers << std::endl;
    std::cout << "\t\t" << "maxDescriptorSetUniformBuffers:                   " << properties.limits.maxDescriptorSetUniformBuffers << std::endl;
    std::cout << "\t\t" << "maxDescriptorSetUniformBuffersDynamic:            " << properties.limits.maxDescriptorSetUniformBuffersDynamic << std::endl;
    std::cout << "\t\t" << "maxDescriptorSetStorageBuffers:                   " << properties.limits.maxDescriptorSetStorageBuffers << std::endl;
    std::cout << "\t\t" << "maxDescriptorSetStorageBuffersDynamic:            " << properties.limits.maxDescriptorSetStorageBuffersDynamic << std::endl;
    std::cout << "\t\t" << "maxDescriptorSetSampledImages:                    " << properties.limits.maxDescriptorSetSampledImages << std::endl;
    std::cout << "\t\t" << "maxDescriptorSetStorageImages:                    " << properties.limits.maxDescriptorSetStorageImages << std::endl;
    std::cout << "\t\t" << "maxDescriptorSetInputAttachments:                 " << properties.limits.maxDescriptorSetInputAttachments << std::endl;
    std::cout << "\t\t" << "maxVertexInputAttributes:                         " << properties.limits.maxVertexInputAttributes << std::endl;
    std::cout << "\t\t" << "maxVertexInputBindings:                           " << properties.limits.maxVertexInputBindings << std::endl;
    std::cout << "\t\t" << "maxVertexInputAttributeOffset:                    " << properties.limits.maxVertexInputAttributeOffset << std::endl;
    std::cout << "\t\t" << "maxVertexInputBindingStride:                      " << properties.limits.maxVertexInputBindingStride << std::endl;
    std::cout << "\t\t" << "maxVertexOutputComponents:                        " << properties.limits.maxVertexOutputComponents << std::endl;
    std::cout << "\t\t" << "maxTessellationGenerationLevel:                   " << properties.limits.maxTessellationGenerationLevel << std::endl;
    std::cout << "\t\t" << "maxTessellationPatchSize:                         " << properties.limits.maxTessellationPatchSize << std::endl;
    std::cout << "\t\t" << "maxTessellationControlPerVertexInputComponents:   " << properties.limits.maxTessellationControlPerVertexInputComponents << std::endl;
    std::cout << "\t\t" << "maxTessellationControlPerVertexOutputComponents:  " << properties.limits.maxTessellationControlPerVertexOutputComponents << std::endl;
    std::cout << "\t\t" << "maxTessellationControlPerPatchOutputComponents:   " << properties.limits.maxTessellationControlPerPatchOutputComponents << std::endl;
    std::cout << "\t\t" << "maxTessellationControlTotalOutputComponents:      " << properties.limits.maxTessellationControlTotalOutputComponents << std::endl;
    std::cout << "\t\t" << "maxTessellationEvaluationInputComponents:         " << properties.limits.maxTessellationEvaluationInputComponents << std::endl;
    std::cout << "\t\t" << "maxTessellationEvaluationOutputComponents:        " << properties.limits.maxTessellationEvaluationOutputComponents << std::endl;
    std::cout << "\t\t" << "maxGeometryShaderInvocations:                     " << properties.limits.maxGeometryShaderInvocations << std::endl;
    std::cout << "\t\t" << "maxGeometryInputComponents:                       " << properties.limits.maxGeometryInputComponents << std::endl;
    std::cout << "\t\t" << "maxGeometryOutputComponents:                      " << properties.limits.maxGeometryOutputComponents << std::endl;
    std::cout << "\t\t" << "maxGeometryOutputVertices:                        " << properties.limits.maxGeometryOutputVertices << std::endl;
    std::cout << "\t\t" << "maxGeometryTotalOutputComponents:                 " << properties.limits.maxGeometryTotalOutputComponents << std::endl;
    std::cout << "\t\t" << "maxFragmentInputComponents:                       " << properties.limits.maxFragmentInputComponents << std::endl;
    std::cout << "\t\t" << "maxFragmentOutputAttachments:                     " << properties.limits.maxFragmentOutputAttachments << std::endl;
    std::cout << "\t\t" << "maxFragmentDualSrcAttachments:                    " << properties.limits.maxFragmentDualSrcAttachments << std::endl;
    std::cout << "\t\t" << "maxFragmentCombinedOutputResources:               " << properties.limits.maxFragmentCombinedOutputResources << std::endl;
    std::cout << "\t\t" << "maxComputeSharedMemorySize:                       " << properties.limits.maxComputeSharedMemorySize << std::endl;
    //<< "\t\t" <<maxComputeWorkGroupCount[3]:                              be" << properties.limits.maxComputeWorkGroupCount[3]:;;
    std::cout << "\t\t" << "maxComputeWorkGroupInvocations:                   " << properties.limits.maxComputeWorkGroupInvocations << std::endl;
    //<< "\t\t" <<maxComputeWorkGroupSize[3]:                              be" << properties.limits.maxComputeWorkGroupSize[3]:;
    std::cout << "\t\t" << "subPixelPrecisionBits:                            " << properties.limits.subPixelPrecisionBits << std::endl;
    std::cout << "\t\t" << "subTexelPrecisionBits:                            " << properties.limits.subTexelPrecisionBits << std::endl;
    std::cout << "\t\t" << "mipmapPrecisionBits:                              " << properties.limits.mipmapPrecisionBits << std::endl;
    std::cout << "\t\t" << "maxDrawIndexedIndexValue:                         " << properties.limits.maxDrawIndexedIndexValue << std::endl;
    std::cout << "\t\t" << "maxDrawIndirectCount:                             " << properties.limits.maxDrawIndirectCount << std::endl;
    std::cout << "\t\t" << "maxSamplerLodBias:                                " << properties.limits.maxSamplerLodBias << std::endl;
    std::cout << "\t\t" << "maxSamplerAnisotropy:                             " << properties.limits.maxSamplerAnisotropy << std::endl;
    std::cout << "\t\t" << "maxViewports:                                     " << properties.limits.maxViewports << std::endl;
    std::cout << "\t\t" << "maxViewportDimensions[2]:                         " << properties.limits.maxViewportDimensions[0] << ", " << properties.limits.maxViewportDimensions[1] << std::endl;
    std::cout << "\t\t" << "viewportBoundsRange[2]:                           " << properties.limits.viewportBoundsRange[0] << ", " << properties.limits.viewportBoundsRange[1] << std::endl;
    std::cout << "\t\t" << "viewportSubPixelBits:                             " << properties.limits.viewportSubPixelBits << std::endl;
    std::cout << "\t\t" << "minMemoryMapAlignment:                            " << properties.limits.minMemoryMapAlignment << std::endl;
    std::cout << "\t\t" << "minTexelBufferOffsetAlignment:                    " << properties.limits.minTexelBufferOffsetAlignment << std::endl;
    std::cout << "\t\t" << "minUniformBufferOffsetAlignment:                  " << properties.limits.minUniformBufferOffsetAlignment << std::endl;
    std::cout << "\t\t" << "minStorageBufferOffsetAlignment:                  " << properties.limits.minStorageBufferOffsetAlignment << std::endl;
    std::cout << "\t\t" << "minTexelOffset:                                   " << properties.limits.minTexelOffset << std::endl;
    std::cout << "\t\t" << "maxTexelOffset:                                   " << properties.limits.maxTexelOffset << std::endl;
    std::cout << "\t\t" << "minTexelGatherOffset:                             " << properties.limits.minTexelGatherOffset << std::endl;
    std::cout << "\t\t" << "maxTexelGatherOffset:                             " << properties.limits.maxTexelGatherOffset << std::endl;
    std::cout << "\t\t" << "minInterpolationOffset:                           " << properties.limits.minInterpolationOffset << std::endl;
    std::cout << "\t\t" << "maxInterpolationOffset:                           " << properties.limits.maxInterpolationOffset << std::endl;
    std::cout << "\t\t" << "subPixelInterpolationOffsetBits:                  " << properties.limits.subPixelInterpolationOffsetBits << std::endl;
    std::cout << "\t\t" << "maxFramebufferWidth:                              " << properties.limits.maxFramebufferWidth << std::endl;
    std::cout << "\t\t" << "maxFramebufferHeight:                             " << properties.limits.maxFramebufferHeight << std::endl;
    std::cout << "\t\t" << "maxFramebufferLayers:                             " << properties.limits.maxFramebufferLayers << std::endl;
    std::cout << "\t\t" << "framebufferColorSampleCounts:                     " << properties.limits.framebufferColorSampleCounts << std::endl;
    std::cout << "\t\t" << "framebufferDepthSampleCounts:                     " << properties.limits.framebufferDepthSampleCounts << std::endl;
    std::cout << "\t\t" << "framebufferStencilSampleCounts:                   " << properties.limits.framebufferStencilSampleCounts << std::endl;
    std::cout << "\t\t" << "framebufferNoAttachmentsSampleCounts:             " << properties.limits.framebufferNoAttachmentsSampleCounts << std::endl;
    std::cout << "\t\t" << "maxColorAttachments:                              " << properties.limits.maxColorAttachments << std::endl;
    std::cout << "\t\t" << "sampledImageColorSampleCounts:                    " << properties.limits.sampledImageColorSampleCounts << std::endl;
    std::cout << "\t\t" << "sampledImageIntegerSampleCounts:                  " << properties.limits.sampledImageIntegerSampleCounts << std::endl;
    std::cout << "\t\t" << "sampledImageDepthSampleCounts:                    " << properties.limits.sampledImageDepthSampleCounts << std::endl;
    std::cout << "\t\t" << "sampledImageStencilSampleCounts:                  " << properties.limits.sampledImageStencilSampleCounts << std::endl;
    std::cout << "\t\t" << "storageImageSampleCounts:                         " << properties.limits.storageImageSampleCounts << std::endl;
    std::cout << "\t\t" << "maxSampleMaskWords:                               " << properties.limits.maxSampleMaskWords << std::endl;
    std::cout << "\t\t" << "timestampComputeAndGraphics:                      " << properties.limits.timestampComputeAndGraphics << std::endl;
    std::cout << "\t\t" << "timestampPeriod:                                  " << properties.limits.timestampPeriod << std::endl;
    std::cout << "\t\t" << "maxClipDistances:                                 " << properties.limits.maxClipDistances << std::endl;
    std::cout << "\t\t" << "maxCullDistances:                                 " << properties.limits.maxCullDistances << std::endl;
    std::cout << "\t\t" << "maxCombinedClipAndCullDistances:                  " << properties.limits.maxCombinedClipAndCullDistances << std::endl;
    std::cout << "\t\t" << "discreteQueuePriorities:                          " << properties.limits.discreteQueuePriorities << std::endl;
    std::cout << "\t\t" << "pointSizeRange[2]:                                " << properties.limits.pointSizeRange[0] << ", " << properties.limits.pointSizeRange[1] << std::endl;
    std::cout << "\t\t" << "lineWidthRange[2]:                                " << properties.limits.lineWidthRange[0] << ", " << properties.limits.lineWidthRange[1] << std::endl;
    std::cout << "\t\t" << "pointSizeGranularity:                             " << properties.limits.pointSizeGranularity << std::endl;
    std::cout << "\t\t" << "lineWidthGranularity:                             " << properties.limits.lineWidthGranularity << std::endl;
    std::cout << "\t\t" << "strictLines:                                      " << properties.limits.strictLines << std::endl;
    std::cout << "\t\t" << "standardSampleLocations:                          " << properties.limits.standardSampleLocations << std::endl;
    std::cout << "\t\t" << "optimalBufferCopyOffsetAlignment:                 " << properties.limits.optimalBufferCopyOffsetAlignment << std::endl;
    std::cout << "\t\t" << "optimalBufferCopyRowPitchAlignment:               " << properties.limits.optimalBufferCopyRowPitchAlignment << std::endl;
    std::cout << "\t\t" << "nonCoherentAtomSize:                              " << properties.limits.nonCoherentAtomSize << std::endl << std::endl;

    std::cout << "\t" << "Sparce Properties: " << std::endl;
    std::cout << "\t\t" << "residencyAlignedMipSize: " << properties.sparseProperties.residencyAlignedMipSize << std::endl;
    std::cout << "\t\t" << "residencyNonResidentStrict: " << properties.sparseProperties.residencyNonResidentStrict << std::endl;
    std::cout << "\t\t" << "residencyStandard2DBlockShape: " << properties.sparseProperties.residencyStandard2DBlockShape << std::endl;
    std::cout << "\t\t" << "residencyStandard2DMultisampleBlockShape: " << properties.sparseProperties.residencyStandard2DMultisampleBlockShape << std::endl;
    std::cout << "\t\t" << "residencyStandard3DBlockShape: " << properties.sparseProperties.residencyStandard3DBlockShape << std::endl << std::endl;


    auto const& memoryProperties2 = deviceProperties.memoryProperties2; 
    auto const& memoryProperties = memoryProperties2.memoryProperties;
    auto const& memoryBudgetPropertiesEXT = *((VkPhysicalDeviceMemoryBudgetPropertiesEXT*)memoryProperties2.pNext);
    
    std::cout << "\t" << "Memory Properties: " << std::endl;
    std::cout << "\t\t" << "memoryHeapCount: " << memoryProperties.memoryHeapCount << std::endl;
    for (auto i = 0u; i < memoryProperties.memoryHeapCount; ++i) {
        std::cout << "\t\t\tHeap " << i << ": " << std::endl;
        std::cout << "\t\t\t\tsize: " << memoryProperties.memoryHeaps[i].size << std::endl;
        if (IsAPI13SupportedByPhysicalDevice(properties))
        {
            std::cout << "\t\t\t\tbudget_EXT: " << memoryBudgetPropertiesEXT.heapBudget[i] << std::endl;
            std::cout << "\t\t\t\tusage_EXT: " << memoryBudgetPropertiesEXT.heapUsage[i] << std::endl;
        }
        std::cout << "\t\t\t\tflag VK_MEMORY_HEAP_DEVICE_LOCAL_BIT: " << static_cast<bool>(memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) << std::endl;
        std::cout << "\t\t\t\tflag VK_MEMORY_HEAP_MULTI_INSTANCE_BIT: " << static_cast<bool>(memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT) << std::endl;
        std::cout << "\t\t\t\tflag VK_MEMORY_HEAP_MULTI_INSTANCE_BIT_KHR: " << static_cast<bool>(memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT_KHR) << std::endl << std::endl;

    }



    std::cout << "\t\t" << "memoryTypeCount: " << memoryProperties.memoryTypeCount << std::endl;
    for (auto i = 0u; i < memoryProperties.memoryTypeCount; ++i) {
        std::cout << "\t\t\tMemory type " << i << ": " << std::endl;
        std::cout << "\t\t\theap: " << memoryProperties.memoryTypes[i].heapIndex << std::endl;
        std::cout << "\t\t\tflags: " << std::endl;
        std::cout << "\t\t\t\tVK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT: " << static_cast<bool>(memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) << std::endl;
        std::cout << "\t\t\t\tVK_MEMORY_PROPERTY_HOST_VISIBLE_BIT: " << static_cast<bool>(memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) << std::endl;
        std::cout << "\t\t\t\tVK_MEMORY_PROPERTY_HOST_COHERENT_BIT: " << static_cast<bool>(memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) << std::endl;
        std::cout << "\t\t\t\tVK_MEMORY_PROPERTY_HOST_CACHED_BIT: " << static_cast<bool>(memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) << std::endl;
        std::cout << "\t\t\t\tVK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT: " << static_cast<bool>(memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) << std::endl;
        std::cout << "\t\t\t\tVK_MEMORY_PROPERTY_PROTECTED_BIT: " << static_cast<bool>(memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT) << std::endl << std::endl;
    }


    auto const& queueFamilyProperties = deviceProperties.queueFamilyProperties;

    std::cout << "\tQueue Family Properties: " << std::endl;
    for (auto i = 0u; i < queueFamilyProperties.size(); ++i) {
        std::cout << "\t\tqueue properties " << i << ": " << std::endl;
        std::cout << "\t\t\tqueueCount: " << queueFamilyProperties[i].queueCount << std::endl;
        std::cout << "\t\t\tVK_QUEUE_GRAPHICS_BIT: " << static_cast<bool>(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) << std::endl;
        std::cout << "\t\t\tVK_QUEUE_COMPUTE_BIT: " << static_cast<bool>(queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) << std::endl;
        std::cout << "\t\t\tVK_QUEUE_TRANSFER_BIT: " << static_cast<bool>(queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) << std::endl;
        std::cout << "\t\t\tVK_QUEUE_SPARSE_BINDING_BIT: " << static_cast<bool>(queueFamilyProperties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) << std::endl;
        std::cout << "\t\t\tVK_QUEUE_PROTECTED_BIT: " << static_cast<bool>(queueFamilyProperties[i].queueFlags & VK_QUEUE_PROTECTED_BIT) << std::endl << std::endl;
    }


    auto const& extensionProperties = deviceProperties.extensionProperties;

    std::cout << "\tDevice Extension Properties: " << std::endl;
    for (auto i = 0u; i < extensionProperties.size(); ++i) {
        std::cout << "\t\textensionProperties " << i << ": " << std::endl;
        std::cout << "\t\t\textensionName: " << extensionProperties[i].extensionName << std::endl;
        std::cout << "\t\t\tspecVersion: " << extensionProperties[i].specVersion << std::endl << std::endl;
        
    }


    auto const& features = deviceProperties.features2.features;

    std::cout << "\tDevice Features:" << std::endl;
    std::cout << "\t\t" << "robustBufferAccess: " << features.robustBufferAccess << std::endl;
    std::cout << "\t\t" << "fullDrawIndexUint32: " << features.fullDrawIndexUint32 << std::endl;
    std::cout << "\t\t" << "imageCubeArray: " << features.imageCubeArray << std::endl;
    std::cout << "\t\t" << "independentBlend: " << features.independentBlend << std::endl;
    std::cout << "\t\t" << "geometryShader: " << features.geometryShader << std::endl;
    std::cout << "\t\t" << "tessellationShader: " << features.tessellationShader << std::endl;
    std::cout << "\t\t" << "sampleRateShading: " << features.sampleRateShading << std::endl;
    std::cout << "\t\t" << "dualSrcBlend: " << features.dualSrcBlend << std::endl;
    std::cout << "\t\t" << "logicOp: " << features.logicOp << std::endl;
    std::cout << "\t\t" << "multiDrawIndirect: " << features.multiDrawIndirect << std::endl;
    std::cout << "\t\t" << "drawIndirectFirstInstance: " << features.drawIndirectFirstInstance << std::endl;
    std::cout << "\t\t" << "depthClamp: " << features.depthClamp << std::endl;
    std::cout << "\t\t" << "depthBiasClamp: " << features.depthBiasClamp << std::endl;
    std::cout << "\t\t" << "fillModeNonSolid: " << features.fillModeNonSolid << std::endl;
    std::cout << "\t\t" << "depthBounds: " << features.depthBounds << std::endl;
    std::cout << "\t\t" << "wideLines: " << features.wideLines << std::endl;
    std::cout << "\t\t" << "largePoints: " << features.largePoints << std::endl;
    std::cout << "\t\t" << "alphaToOne: " << features.alphaToOne << std::endl;
    std::cout << "\t\t" << "multiViewport: " << features.multiViewport << std::endl;
    std::cout << "\t\t" << "samplerAnisotropy: " << features.samplerAnisotropy << std::endl;
    std::cout << "\t\t" << "textureCompressionETC2: " << features.textureCompressionETC2 << std::endl;
    std::cout << "\t\t" << "textureCompressionASTC_LDR: " << features.textureCompressionASTC_LDR << std::endl;
    std::cout << "\t\t" << "textureCompressionBC: " << features.textureCompressionBC << std::endl;
    std::cout << "\t\t" << "occlusionQueryPrecise: " << features.occlusionQueryPrecise << std::endl;
    std::cout << "\t\t" << "pipelineStatisticsQuery: " << features.pipelineStatisticsQuery << std::endl;
    std::cout << "\t\t" << "vertexPipelineStoresAndAtomics: " << features.vertexPipelineStoresAndAtomics << std::endl;
    std::cout << "\t\t" << "fragmentStoresAndAtomics: " << features.fragmentStoresAndAtomics << std::endl;
    std::cout << "\t\t" << "shaderTessellationAndGeometryPointSize: " << features.shaderTessellationAndGeometryPointSize << std::endl;
    std::cout << "\t\t" << "shaderImageGatherExtended: " << features.shaderImageGatherExtended << std::endl;
    std::cout << "\t\t" << "shaderStorageImageExtendedFormats: " << features.shaderStorageImageExtendedFormats << std::endl;
    std::cout << "\t\t" << "shaderStorageImageMultisample: " << features.shaderStorageImageMultisample << std::endl;
    std::cout << "\t\t" << "shaderStorageImageReadWithoutFormat: " << features.shaderStorageImageReadWithoutFormat << std::endl;
    std::cout << "\t\t" << "shaderStorageImageWriteWithoutFormat: " << features.shaderStorageImageWriteWithoutFormat << std::endl;
    std::cout << "\t\t" << "shaderUniformBufferArrayDynamicIndexing: " << features.shaderUniformBufferArrayDynamicIndexing << std::endl;
    std::cout << "\t\t" << "shaderSampledImageArrayDynamicIndexing: " << features.shaderSampledImageArrayDynamicIndexing << std::endl;
    std::cout << "\t\t" << "shaderStorageBufferArrayDynamicIndexing: " << features.shaderStorageBufferArrayDynamicIndexing << std::endl;
    std::cout << "\t\t" << "shaderStorageImageArrayDynamicIndexing: " << features.shaderStorageImageArrayDynamicIndexing << std::endl;
    std::cout << "\t\t" << "shaderClipDistance: " << features.shaderClipDistance << std::endl;
    std::cout << "\t\t" << "shaderCullDistance: " << features.shaderCullDistance << std::endl;
    std::cout << "\t\t" << "shaderFloat64: " << features.shaderFloat64 << std::endl;
    std::cout << "\t\t" << "shaderInt64: " << features.shaderInt64 << std::endl;
    std::cout << "\t\t" << "shaderInt16: " << features.shaderInt16 << std::endl;
    std::cout << "\t\t" << "shaderResourceResidency: " << features.shaderResourceResidency << std::endl;
    std::cout << "\t\t" << "shaderResourceMinLod: " << features.shaderResourceMinLod << std::endl;
    std::cout << "\t\t" << "sparseBinding: " << features.sparseBinding << std::endl;
    std::cout << "\t\t" << "sparseResidencyBuffer: " << features.sparseResidencyBuffer << std::endl;
    std::cout << "\t\t" << "sparseResidencyImage2D: " << features.sparseResidencyImage2D << std::endl;
    std::cout << "\t\t" << "sparseResidencyImage3D: " << features.sparseResidencyImage3D << std::endl;
    std::cout << "\t\t" << "sparseResidency2Samples: " << features.sparseResidency2Samples << std::endl;
    std::cout << "\t\t" << "sparseResidency4Samples: " << features.sparseResidency4Samples << std::endl;
    std::cout << "\t\t" << "sparseResidency8Samples: " << features.sparseResidency8Samples << std::endl;
    std::cout << "\t\t" << "sparseResidency16Samples: " << features.sparseResidency16Samples << std::endl;
    std::cout << "\t\t" << "sparseResidencyAliased: " << features.sparseResidencyAliased << std::endl;
    std::cout << "\t\t" << "variableMultisampleRate: " << features.variableMultisampleRate << std::endl;
    std::cout << "\t\t" << "inheritedQueries: " << features.inheritedQueries << std::endl << std::endl;

#ifndef DRE_COMPILE_FOR_RENDERDOC
    VkPhysicalDeviceExtendedDynamicState3FeaturesEXT const& dynamicStateFeatures = *((VkPhysicalDeviceExtendedDynamicState3FeaturesEXT const*)deviceProperties.features2.pNext);
    VkPhysicalDeviceVulkan12Features const& vulkan12Features = *((VkPhysicalDeviceVulkan12Features const*)dynamicStateFeatures.pNext);
#else
    VkPhysicalDeviceVulkan12Features const& vulkan12Features = *((VkPhysicalDeviceVulkan12Features const*)deviceProperties.features2.pNext);
#endif
    std::cout << "\tDescriptor Indexing Feataures:" << std::endl;
    std::cout << "\t\t" << "shaderInputAttachmentArrayDynamicIndexing: "        << vulkan12Features.shaderInputAttachmentArrayDynamicIndexing << std::endl;
    std::cout << "\t\t" << "shaderUniformTexelBufferArrayDynamicIndexing: "     << vulkan12Features.shaderUniformTexelBufferArrayDynamicIndexing << std::endl;
    std::cout << "\t\t" << "shaderStorageTexelBufferArrayDynamicIndexing: "     << vulkan12Features.shaderStorageTexelBufferArrayDynamicIndexing << std::endl;
    std::cout << "\t\t" << "shaderUniformBufferArrayNonUniformIndexing: "       << vulkan12Features.shaderUniformBufferArrayNonUniformIndexing << std::endl;
    std::cout << "\t\t" << "shaderSampledImageArrayNonUniformIndexing: "        << vulkan12Features.shaderSampledImageArrayNonUniformIndexing << std::endl;
    std::cout << "\t\t" << "shaderStorageBufferArrayNonUniformIndexing: "       << vulkan12Features.shaderStorageBufferArrayNonUniformIndexing << std::endl;
    std::cout << "\t\t" << "shaderStorageImageArrayNonUniformIndexing: "        << vulkan12Features.shaderStorageImageArrayNonUniformIndexing << std::endl;
    std::cout << "\t\t" << "shaderInputAttachmentArrayNonUniformIndexing: "     << vulkan12Features.shaderInputAttachmentArrayNonUniformIndexing << std::endl;
    std::cout << "\t\t" << "shaderUniformTexelBufferArrayNonUniformIndexing: "  << vulkan12Features.shaderUniformTexelBufferArrayNonUniformIndexing << std::endl;
    std::cout << "\t\t" << "shaderStorageTexelBufferArrayNonUniformIndexing: "  << vulkan12Features.shaderStorageTexelBufferArrayNonUniformIndexing << std::endl;
    std::cout << "\t\t" << "descriptorBindingUniformBufferUpdateAfterBind: "    << vulkan12Features.descriptorBindingUniformBufferUpdateAfterBind << std::endl;
    std::cout << "\t\t" << "descriptorBindingSampledImageUpdateAfterBind: "     << vulkan12Features.descriptorBindingSampledImageUpdateAfterBind << std::endl;
    std::cout << "\t\t" << "descriptorBindingStorageImageUpdateAfterBind: "     << vulkan12Features.descriptorBindingStorageImageUpdateAfterBind << std::endl;
    std::cout << "\t\t" << "descriptorBindingStorageBufferUpdateAfterBind: "    << vulkan12Features.descriptorBindingStorageBufferUpdateAfterBind << std::endl;
    std::cout << "\t\t" << "descriptorBindingUniformTexelBufferUpdateAfterBind: " << vulkan12Features.descriptorBindingUniformTexelBufferUpdateAfterBind << std::endl;
    std::cout << "\t\t" << "descriptorBindingStorageTexelBufferUpdateAfterBind: " << vulkan12Features.descriptorBindingStorageTexelBufferUpdateAfterBind << std::endl;
    std::cout << "\t\t" << "descriptorBindingUpdateUnusedWhilePending: "        << vulkan12Features.descriptorBindingUpdateUnusedWhilePending << std::endl;
    std::cout << "\t\t" << "descriptorBindingPartiallyBound: "                  << vulkan12Features.descriptorBindingPartiallyBound << std::endl;
    std::cout << "\t\t" << "descriptorBindingVariableDescriptorCount: "         << vulkan12Features.descriptorBindingVariableDescriptorCount << std::endl;
    std::cout << "\t\t" << "runtimeDescriptorArray: "                           << vulkan12Features.runtimeDescriptorArray << std::endl << std::endl;



    VkPhysicalDeviceVulkan12Properties const& vulkan12Properties = *((VkPhysicalDeviceVulkan12Properties*)deviceProperties.properties2.pNext);
    VkPhysicalDeviceDescriptorIndexingProperties const& descriptorIndexingProperties = *((VkPhysicalDeviceDescriptorIndexingProperties const*)vulkan12Properties.pNext);
    std::cout << "\tDescriptor Indexing Properties:" << std::endl;
    std::cout << "\t\t" << "maxUpdateAfterBindDescriptorsInAllPools: "              << descriptorIndexingProperties.maxUpdateAfterBindDescriptorsInAllPools << std::endl;
    std::cout << "\t\t" << "shaderUniformBufferArrayNonUniformIndexingNative: "     << descriptorIndexingProperties.shaderUniformBufferArrayNonUniformIndexingNative << std::endl;
    std::cout << "\t\t" << "shaderSampledImageArrayNonUniformIndexingNative: "      << descriptorIndexingProperties.shaderSampledImageArrayNonUniformIndexingNative << std::endl;
    std::cout << "\t\t" << "shaderStorageBufferArrayNonUniformIndexingNative: "     << descriptorIndexingProperties.shaderStorageBufferArrayNonUniformIndexingNative << std::endl;
    std::cout << "\t\t" << "shaderStorageImageArrayNonUniformIndexingNative: "      << descriptorIndexingProperties.shaderStorageImageArrayNonUniformIndexingNative << std::endl;
    std::cout << "\t\t" << "shaderInputAttachmentArrayNonUniformIndexingNative: "   << descriptorIndexingProperties.shaderInputAttachmentArrayNonUniformIndexingNative << std::endl;
    std::cout << "\t\t" << "robustBufferAccessUpdateAfterBind: "                    << descriptorIndexingProperties.robustBufferAccessUpdateAfterBind << std::endl;
    std::cout << "\t\t" << "quadDivergentImplicitLod: "                             << descriptorIndexingProperties.quadDivergentImplicitLod << std::endl;
    std::cout << "\t\t" << "maxPerStageDescriptorUpdateAfterBindSamplers: "         << descriptorIndexingProperties.maxPerStageDescriptorUpdateAfterBindSamplers << std::endl;
    std::cout << "\t\t" << "maxPerStageDescriptorUpdateAfterBindUniformBuffers: "   << descriptorIndexingProperties.maxPerStageDescriptorUpdateAfterBindUniformBuffers << std::endl;
    std::cout << "\t\t" << "maxPerStageDescriptorUpdateAfterBindStorageBuffers: "   << descriptorIndexingProperties.maxPerStageDescriptorUpdateAfterBindStorageBuffers << std::endl;
    std::cout << "\t\t" << "maxPerStageDescriptorUpdateAfterBindSampledImages: "    << descriptorIndexingProperties.maxPerStageDescriptorUpdateAfterBindSampledImages << std::endl;
    std::cout << "\t\t" << "maxPerStageDescriptorUpdateAfterBindStorageImages: "    << descriptorIndexingProperties.maxPerStageDescriptorUpdateAfterBindStorageImages << std::endl;
    std::cout << "\t\t" << "maxPerStageDescriptorUpdateAfterBindInputAttachments: " << descriptorIndexingProperties.maxPerStageDescriptorUpdateAfterBindInputAttachments << std::endl;
    std::cout << "\t\t" << "maxPerStageUpdateAfterBindResources: "                  << descriptorIndexingProperties.maxPerStageUpdateAfterBindResources << std::endl;
    std::cout << "\t\t" << "maxDescriptorSetUpdateAfterBindSamplers: "              << descriptorIndexingProperties.maxDescriptorSetUpdateAfterBindSamplers << std::endl;
    std::cout << "\t\t" << "maxDescriptorSetUpdateAfterBindUniformBuffers: "        << descriptorIndexingProperties.maxDescriptorSetUpdateAfterBindUniformBuffers << std::endl;
    std::cout << "\t\t" << "maxDescriptorSetUpdateAfterBindUniformBuffersDynamic: " << descriptorIndexingProperties.maxDescriptorSetUpdateAfterBindUniformBuffersDynamic << std::endl;
    std::cout << "\t\t" << "maxDescriptorSetUpdateAfterBindStorageBuffers: "        << descriptorIndexingProperties.maxDescriptorSetUpdateAfterBindStorageBuffers << std::endl;
    std::cout << "\t\t" << "maxDescriptorSetUpdateAfterBindStorageBuffersDynamic: " << descriptorIndexingProperties.maxDescriptorSetUpdateAfterBindStorageBuffersDynamic << std::endl;
    std::cout << "\t\t" << "maxDescriptorSetUpdateAfterBindSampledImages: "         << descriptorIndexingProperties.maxDescriptorSetUpdateAfterBindSampledImages << std::endl;
    std::cout << "\t\t" << "maxDescriptorSetUpdateAfterBindStorageImages: "         << descriptorIndexingProperties.maxDescriptorSetUpdateAfterBindStorageImages << std::endl;
    std::cout << "\t\t" << "maxDescriptorSetUpdateAfterBindInputAttachments: "      << descriptorIndexingProperties.maxDescriptorSetUpdateAfterBindInputAttachments << std::endl << std::endl;
}

}