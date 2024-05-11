#include <vk_wrapper\resources\ResourcesController.hpp>

#include <algorithm>

#include <foundation\string\InplaceString.hpp>

#include <vk_wrapper\memory\MemoryController.hpp>
#include <vk_wrapper\LogicalDevice.hpp>
#include <vk_wrapper\Helper.hpp>
#include <vk_wrapper\Tools.hpp>
#include <vk_wrapper\Constant.hpp>

namespace VKW
{

ResourcesController::ResourcesController()
    : table_{ nullptr }
    , device_{ nullptr }
    , memoryController_{ nullptr }
{
}

ResourcesController::ResourcesController(ImportTable* table, LogicalDevice* device, MemoryController* memoryController)
    : table_{ table }
    , device_{ device }
    , memoryController_{ memoryController }
{

}

ResourcesController::ResourcesController(ResourcesController&& rhs)
{
    operator=(std::move(rhs));
}

ResourcesController& ResourcesController::operator=(ResourcesController&& rhs)
{
    std::swap(table_, rhs.table_);
    std::swap(device_, rhs.device_);
    std::swap(memoryController_, rhs.memoryController_);

    std::swap(buffers_, rhs.buffers_);
    std::swap(images_, rhs.images_);
    std::swap(imageViewMap_, rhs.imageViewMap_);
    
    return *this;
}

ResourcesController::~ResourcesController()
{
    VkDevice const device = device_->Handle();
    
    for (auto& bufferResource : buffers_) {
        table_->vkDestroyBuffer(device, bufferResource->handle_, nullptr);
        memoryController_->ReleaseMemoryRegion(bufferResource->memory_);
        delete bufferResource;
    }

    for (auto& imageResource : images_) {
        table_->vkDestroyImage(device, imageResource->handle_, nullptr);
        memoryController_->ReleaseMemoryRegion(imageResource->memory_);
        delete imageResource;
    }
}

BufferResource* ResourcesController::CreateBuffer(std::uint32_t size, BufferUsage usage, char const* name)
{
    VkBufferCreateInfo vkBufferCreateInfo;
    vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vkBufferCreateInfo.pNext = nullptr;
    vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkBufferCreateInfo.queueFamilyIndexCount = 0;
    vkBufferCreateInfo.pQueueFamilyIndices = nullptr;
    vkBufferCreateInfo.size = size;
    vkBufferCreateInfo.flags = VK_FLAGS_NONE;
    vkBufferCreateInfo.usage = VK_FLAGS_NONE; // temp value, assigned below


    MemoryPageRegionDesc regionDesc;

    switch (usage)
    {
    case BufferUsage::VERTEX_INDEX:
        regionDesc.memoryClass_ = MemoryClass::DeviceFast;
        vkBufferCreateInfo.usage = (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        break;
    case BufferUsage::VERTEX_INDEX_WRITABLE:
        regionDesc.memoryClass_ = MemoryClass::CpuStaging;
        vkBufferCreateInfo.usage = (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        break;
    case BufferUsage::UNIFORM:
        regionDesc.memoryClass_ = MemoryClass::CpuUniform;
        vkBufferCreateInfo.usage = (VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        break;
    case BufferUsage::UPLOAD_BUFFER:
        regionDesc.memoryClass_ = MemoryClass::CpuStaging;
        vkBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
#if defined(DRE_GET_BUFFER_ADDRESS)
        vkBufferCreateInfo.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
#endif
        break;
    case BufferUsage::READBACK_BUFFER:
        regionDesc.memoryClass_ = MemoryClass::CpuReadback;
        vkBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        break;
    case BufferUsage::STORAGE:
        regionDesc.memoryClass_ = MemoryClass::DeviceFast;
        vkBufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
#if defined(DRE_GET_BUFFER_ADDRESS)
        vkBufferCreateInfo.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
#endif
        break;
    default:
        assert(false);
    }


    VkBuffer vkBuffer = VK_NULL_HANDLE;
    VK_ASSERT(table_->vkCreateBuffer(device_->Handle(), &vkBufferCreateInfo, nullptr, &vkBuffer));

    VkMemoryRequirements memoryRequirements;
    table_->vkGetBufferMemoryRequirements(device_->Handle(), vkBuffer, &memoryRequirements);

    regionDesc.size_ = memoryRequirements.size;
    regionDesc.alignment_ = memoryRequirements.alignment;
    regionDesc.memoryTypeBits_ = memoryRequirements.memoryTypeBits;
    assert(IsPowerOf2(regionDesc.alignment_) && "Alignemnt is not power of 2!");


    MemoryRegion memoryRegion = memoryController_->AllocateMemoryRegion(regionDesc);
    VK_ASSERT(table_->vkBindBufferMemory(device_->Handle(), vkBuffer, memoryRegion.page_->deviceMemory_, memoryRegion.offset_));

    std::uint64_t gpuAddress = 0;
#if defined(DRE_GET_BUFFER_ADDRESS)
    if (usage == BufferUsage::UPLOAD_BUFFER || usage == BufferUsage::STORAGE)
    {
        VkBufferDeviceAddressInfo addressInfo;
        addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        addressInfo.pNext = nullptr;
        addressInfo.buffer = vkBuffer;
        gpuAddress = table_->vkGetBufferDeviceAddress(device_->Handle(), &addressInfo);
    }
#endif

    BufferResource* resource = new BufferResource{ vkBuffer, size, memoryRegion, gpuAddress };
    buffers_.emplace(resource);

#ifdef DRE_DEBUG
    DRE::String128 nameBuffer{ "BUFFER|" };
    nameBuffer.Append(name);

    VkDebugUtilsObjectNameInfoEXT nameInfo;
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.pNext = nullptr;
    nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
    nameInfo.objectHandle = (std::uint64_t)vkBuffer;
    nameInfo.pObjectName = nameBuffer.GetData();

    VK_ASSERT(table_->vkSetDebugUtilsObjectNameEXT(device_->Handle(), &nameInfo));
#endif

    return resource;
}

ImageResource* ResourcesController::CreateImage(std::uint32_t width, std::uint32_t height, Format format, ImageUsage usage, char const* name)
{
    VkImageCreateInfo info;
    info.sType                  = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.pNext                  = nullptr;
    info.format                 = Format2VK(format);
    info.imageType              = VK_IMAGE_TYPE_2D;
    info.extent.width           = width;
    info.extent.height          = height;
    info.extent.depth           = 1;
    info.arrayLayers            = 1;
    info.mipLevels              = 1;
    info.tiling                 = VK_IMAGE_TILING_OPTIMAL;
    info.samples                = VK_SAMPLE_COUNT_1_BIT;
    info.sharingMode            = VK_SHARING_MODE_EXCLUSIVE;
    info.queueFamilyIndexCount  = 0;
    info.pQueueFamilyIndices    = nullptr;
    info.initialLayout          = VK_IMAGE_LAYOUT_UNDEFINED;
    info.flags                  = VK_FLAGS_NONE;

    MemoryPageRegionDesc memoryDesc;

    switch (usage)
    {
    case ImageUsage::TEXTURE:
        info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        memoryDesc.memoryClass_ = MemoryClass::DeviceFast;
        break;
        
    case ImageUsage::STORAGE_IMAGE:
        info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | 
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        memoryDesc.memoryClass_ = MemoryClass::DeviceFast;
        break;

    case ImageUsage::RENDER_TARGET:
        info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | 
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | 
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        memoryDesc.memoryClass_ = MemoryClass::DeviceFast;
        break;

    case ImageUsage::DEPTH:
    case ImageUsage::STENCIL:
    case ImageUsage::DEPTH_STENCIL:
        info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        memoryDesc.memoryClass_ = MemoryClass::DeviceFast;
        break;

    case ImageUsage::DEPTH_SAMPLED:
        info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        memoryDesc.memoryClass_ = MemoryClass::DeviceFast;
        break;

    case ImageUsage::UPLOAD_IMAGE:
        info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        info.tiling = VK_IMAGE_TILING_LINEAR;
        memoryDesc.memoryClass_ = MemoryClass::CpuStaging;
        break;
    default:
        assert(false && "Non-supported usage for image.");
        break;
    }


    VkImage vkImage = VK_NULL_HANDLE;
    VK_ASSERT(table_->vkCreateImage(device_->Handle(), &info, nullptr, &vkImage));

    VkMemoryRequirements memoryRequirements;
    table_->vkGetImageMemoryRequirements(device_->Handle(), vkImage, &memoryRequirements);

    memoryDesc.size_ = memoryRequirements.size;
    memoryDesc.alignment_ = memoryRequirements.alignment;
    memoryDesc.memoryTypeBits_ = memoryRequirements.memoryTypeBits;

    MemoryRegion memoryRegion = memoryController_->AllocateMemoryRegion(memoryDesc);
    VK_ASSERT(table_->vkBindImageMemory(device_->Handle(), vkImage, memoryRegion.page_->deviceMemory_, memoryRegion.offset_));

    ImageResource* imageResource = new ImageResource{ vkImage, format, width, height, memoryRegion, info };
    images_.emplace(imageResource);

#ifdef DRE_DEBUG
    DRE::String128 nameBuffer{ "IMAGE|" };
    nameBuffer.Append(name);

    VkDebugUtilsObjectNameInfoEXT nameInfo;
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.pNext = nullptr;
    nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
    nameInfo.objectHandle = (std::uint64_t)vkImage;
    nameInfo.pObjectName = nameBuffer.GetData();

    VK_ASSERT(table_->vkSetDebugUtilsObjectNameEXT(device_->Handle(), &nameInfo));
#endif

    return imageResource;
}

void ResourcesController::FreeBuffer(BufferResource* buffer)
{
    auto bufferIt = buffers_.find(buffer);
    assert(bufferIt != buffers_.end() && "Can't free BufferResource.");

    table_->vkDestroyBuffer(device_->Handle(), buffer->handle_, nullptr);
    delete buffer;

    buffers_.erase(bufferIt);
}

void ResourcesController::FreeImage(ImageResource* image)
{
    auto imageIt = images_.find(image);
    assert(imageIt != images_.end() && "Can't free ImageResource");

    auto views = imageViewMap_.equal_range(image);
    for (auto i = views.first; i != views.second; ++i)
    {
        table_->vkDestroyImageView(device_->Handle(), i->second->handle_, nullptr);
        delete i->second;
    }
    imageViewMap_.erase(views.first, views.second);

    table_->vkDestroyImage(device_->Handle(), image->handle_, nullptr);
    delete image;

    images_.erase(imageIt);
}

VkImageViewType ResourcesController::ImageTypeToViewType(VkImageType type, std::uint32_t arrayLayers)
{
    assert(arrayLayers > 0);
    
    if (arrayLayers > 1)
    {
        switch (type)
        {
        case VK_IMAGE_TYPE_1D:
            return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
        case VK_IMAGE_TYPE_2D:
            return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        case VK_IMAGE_TYPE_3D: 
        default:
            return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
        }
    }
    else
    {
        switch (type)
        {
        case VK_IMAGE_TYPE_1D:
            return VK_IMAGE_VIEW_TYPE_1D;
        case VK_IMAGE_TYPE_2D:
            return VK_IMAGE_VIEW_TYPE_2D;
        case VK_IMAGE_TYPE_3D:
            return VK_IMAGE_VIEW_TYPE_3D;
        default:
            return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
        }
    }
}

VkComponentMapping ResourcesController::DefaultComponentMapping()
{
    return VkComponentMapping{ VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
}

ImageResourceView* ResourcesController::ViewImageAs(ImageResource* resource, VkImageSubresourceRange const* subresource, Format const* format, VkImageViewType const* type, VkComponentMapping const* mapping)
{
    VkImageViewCreateInfo viewCreateInfo;
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.pNext = nullptr;
    viewCreateInfo.image = resource->handle_;
    viewCreateInfo.format = format ? Format2VK(*format) : resource->createInfo_.format;
    viewCreateInfo.viewType = type ? *type : ImageTypeToViewType(resource->createInfo_.imageType, resource->createInfo_.arrayLayers);
    viewCreateInfo.components = mapping ? *mapping : DefaultComponentMapping();
    viewCreateInfo.flags = VK_FLAGS_NONE;
    viewCreateInfo.subresourceRange = subresource ? *subresource : VKW::HELPER::DefaultImageSubresourceRange();

    VkImageView handle = VK_NULL_HANDLE;
    VK_ASSERT(table_->vkCreateImageView(device_->Handle(), &viewCreateInfo, nullptr, &handle));

    ImageResourceView* view = new ImageResourceView{ handle , viewCreateInfo, resource };

    imageViewMap_.emplace(resource, view);

    return view;
}

void ResourcesController::FreeImageView(ImageResourceView* view)
{
    auto parentViews = imageViewMap_.equal_range(view->parentResource_);
    assert(parentViews.first != imageViewMap_.end() && "Image view has no parent resource.");
    
    auto viewPair = parentViews.first;
    while (viewPair != parentViews.second)
    {
        if(viewPair->second == view)
            break;

        ++viewPair;
    }

    table_->vkDestroyImageView(device_->Handle(), view->handle_, nullptr);
    delete view;
    imageViewMap_.erase(viewPair);
}

}