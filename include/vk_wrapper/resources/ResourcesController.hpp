#pragma once

#include <foundation\class_features\NonCopyable.hpp>
#include <vk_wrapper\resources\Resource.hpp>

#include <vulkan\vulkan.h>
#include <unordered_set>
#include <unordered_map>

namespace VKW
{

enum class BufferUsage
{
    VERTEX_INDEX,
    VERTEX_INDEX_WRITABLE,
    UNIFORM,
    UPLOAD_BUFFER,
    READBACK_BUFFER,
    STORAGE
};

enum class ImageUsage
{
    TEXTURE,
    STORAGE_IMAGE,
    RENDER_TARGET,
    DEPTH,
    DEPTH_SAMPLED,
    STENCIL,
    DEPTH_STENCIL,
    UPLOAD_IMAGE
};

class ImportTable;
class LogicalDevice;
class MemoryController;

class ResourcesController
    : public NonCopyable
{
public:
    ResourcesController();
    ResourcesController(ImportTable* table, LogicalDevice* device, MemoryController* memoryController);

    ResourcesController(ResourcesController&& rhs);
    ResourcesController& operator=(ResourcesController&& rhs);

    BufferResource* CreateBuffer(std::uint32_t size, BufferUsage usage, char const* name);
    void FreeBuffer(BufferResource* handle);

    ImageResource* CreateImage(std::uint32_t width, std::uint32_t height, Format format, ImageUsage usage, char const* name);
    void FreeImage(ImageResource* handle);

    ImageResourceView* ViewImageAs(
        ImageResource* resource,
        VkImageSubresourceRange const* subresource = nullptr, 
        Format const* format = nullptr, 
        VkImageViewType const* type = nullptr, 
        VkComponentMapping const* mapping = nullptr
    );
    void FreeImageView(ImageResourceView* view);

    ~ResourcesController();

public:
    static VkImageViewType         ImageTypeToViewType(VkImageType type, std::uint32_t arrayLayers);
    static VkComponentMapping      DefaultComponentMapping();

private:
    ImportTable* table_;
    LogicalDevice* device_;

    MemoryController* memoryController_;

    std::unordered_set<BufferResource*> buffers_;
    std::unordered_set<ImageResource*> images_;
    std::unordered_multimap<ImageResource*, ImageResourceView*> imageViewMap_;


};

}