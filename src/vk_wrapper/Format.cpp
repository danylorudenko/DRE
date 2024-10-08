#include <vk_wrapper\Format.hpp>

#include <foundation\Common.hpp>

namespace VKW
{

std::uint8_t FormatSize(Format format)
{
    switch (format)
    {
    case FORMAT_R32_FLOAT:
    case FORMAT_R32_SINT:
    case FORMAT_R32_UINT:
    case FORMAT_R8G8B8A8_UNORM:
        return 4;
    case FORMAT_R32G32_FLOAT:
    case FORMAT_R32G32_SINT:
    case FORMAT_R32G32_UINT:
        return 8;
    case FORMAT_R32G32B32_FLOAT:
    case FORMAT_R32G32B32_SINT:
    case FORMAT_R32G32B32_UINT:
        return 12;
    case FORMAT_R32G32B32A32_FLOAT:
    case FORMAT_R32G32B32A32_SINT:
    case FORMAT_R32G32B32A32_UINT:
        return 16;

    case FORMAT_D16_UNORM:
        return 2;
    case FORMAT_D24_UNORM_S8_UINT:
    case FORMAT_D32_FLOAT:
        return 4;
    default:
        DRE_ASSERT(false, "Vertex VkFormat not supported.");
        return 0;
    }
}

VkImageAspectFlags Format2Aspect(Format format)
{
    switch (format)
    {
    case FORMAT_R8_UNORM:
    case FORMAT_R8G8_UNORM:
    case FORMAT_R8G8B8A8_UNORM:
    case FORMAT_B8G8R8A8_UNORM:
    case FORMAT_R8_SRGB:
    case FORMAT_R8G8_SRGB:
    case FORMAT_R8G8B8A8_SRGB:
    case FORMAT_B8G8R8A8_SRGB:
    case FORMAT_R16_FLOAT:
    case FORMAT_R16G16_FLOAT:
    case FORMAT_R16G16B16_FLOAT:
    case FORMAT_R16G16B16A16_FLOAT:
    case FORMAT_R32_FLOAT:
    case FORMAT_R32G32_FLOAT:
    case FORMAT_R32G32B32_FLOAT:
    case FORMAT_R32G32B32A32_FLOAT:
    case FORMAT_R8_UINT:
    case FORMAT_R8G8_UINT:
    case FORMAT_R8G8B8_UINT:
    case FORMAT_R8G8B8A8_UINT:
    case FORMAT_R8_SINT:
    case FORMAT_R8G8_SINT:
    case FORMAT_R8G8B8_SINT:
    case FORMAT_R8G8B8A8_SINT:
    case FORMAT_R16_UINT:
    case FORMAT_R16G16_UINT:
    case FORMAT_R16G16B16_UINT:
    case FORMAT_R16G16B16A16_UINT:
    case FORMAT_R16_SINT:
    case FORMAT_R16G16_SINT:
    case FORMAT_R16G16B16_SINT:
    case FORMAT_R16G16B16A16_SINT:

    case FORMAT_R32_UINT:
    case FORMAT_R32G32_UINT:
    case FORMAT_R32G32B32_UINT:
    case FORMAT_R32G32B32A32_UINT:

    case FORMAT_R32_SINT:
    case FORMAT_R32G32_SINT:
    case FORMAT_R32G32B32_SINT:
    case FORMAT_R32G32B32A32_SINT:
        return VK_IMAGE_ASPECT_COLOR_BIT;

    case FORMAT_D16_UNORM:
    case FORMAT_D32_FLOAT:
        return VK_IMAGE_ASPECT_DEPTH_BIT;
    case FORMAT_D24_UNORM_S8_UINT:
        return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    default:
        DRE_ASSERT(false, "Vertex VkFormat not supported.");
        return 0;
    }
}

VkFormat Format2VK(Format format)
{
    switch (format)
    {
    case FORMAT_UNDEFINED:
        return VK_FORMAT_UNDEFINED;

    case FORMAT_R8_UNORM:
        return VK_FORMAT_R8_UNORM;
    case FORMAT_R8G8_UNORM:
        return VK_FORMAT_R8G8_UNORM;
    case FORMAT_R8G8B8A8_UNORM:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case FORMAT_B8G8R8A8_UNORM:
        return VK_FORMAT_B8G8R8A8_UNORM;


    case FORMAT_R8_SRGB:
        return VK_FORMAT_R8_SRGB;
    case FORMAT_R8G8_SRGB:
        return VK_FORMAT_R8G8_SRGB;
    case FORMAT_R8G8B8A8_SRGB:
        return VK_FORMAT_R8G8B8A8_SRGB;
    case FORMAT_B8G8R8A8_SRGB:
        return VK_FORMAT_B8G8R8A8_SRGB;

    case FORMAT_R16_FLOAT:
        return VK_FORMAT_R16_SFLOAT;
    case FORMAT_R16G16_FLOAT:
        return VK_FORMAT_R16G16_SFLOAT;
    case FORMAT_R16G16B16_FLOAT:
        return VK_FORMAT_R16G16B16_SFLOAT;
    case FORMAT_R16G16B16A16_FLOAT:
        return VK_FORMAT_R16G16B16A16_SFLOAT;

    case FORMAT_R32_FLOAT:
        return VK_FORMAT_R32_SFLOAT;
    case FORMAT_R32G32_FLOAT:
        return VK_FORMAT_R32G32_SFLOAT;
    case FORMAT_R32G32B32_FLOAT:
        return VK_FORMAT_R32G32B32_SFLOAT;
    case FORMAT_R32G32B32A32_FLOAT:
        return VK_FORMAT_R32G32B32A32_SFLOAT;

    case FORMAT_R8_UINT:
        return VK_FORMAT_R8_UINT;
    case FORMAT_R8G8_UINT:
        return VK_FORMAT_R8G8_UINT;
    case FORMAT_R8G8B8_UINT:
        return VK_FORMAT_R8G8B8_UINT;
    case FORMAT_R8G8B8A8_UINT:
        return VK_FORMAT_R8G8B8A8_UINT;

    case FORMAT_R8_SINT:
        return VK_FORMAT_R8_SINT;
    case FORMAT_R8G8_SINT:
        return VK_FORMAT_R8G8_SINT;
    case FORMAT_R8G8B8_SINT:
        return VK_FORMAT_R8G8B8_SINT;
    case FORMAT_R8G8B8A8_SINT:
        return VK_FORMAT_R8G8B8A8_SINT;

    case FORMAT_R16_UINT:
        return VK_FORMAT_R16_UINT;
    case FORMAT_R16G16_UINT:
        return VK_FORMAT_R16G16_UINT;
    case FORMAT_R16G16B16_UINT:
        return VK_FORMAT_R16G16B16_UINT;
    case FORMAT_R16G16B16A16_UINT:
        return VK_FORMAT_R16G16B16A16_UINT;

    case FORMAT_R16_SINT:
        return VK_FORMAT_R16_SINT;
    case FORMAT_R16G16_SINT:
        return VK_FORMAT_R16G16_SINT;
    case FORMAT_R16G16B16_SINT:
        return VK_FORMAT_R16G16B16_SINT;
    case FORMAT_R16G16B16A16_SINT:
        return VK_FORMAT_R16G16B16A16_SINT;

    case FORMAT_R32_UINT:
        return VK_FORMAT_R32_UINT;
    case FORMAT_R32G32_UINT:
        return VK_FORMAT_R32G32_UINT;
    case FORMAT_R32G32B32_UINT:
        return VK_FORMAT_R32G32B32_UINT;
    case FORMAT_R32G32B32A32_UINT:
        return VK_FORMAT_R32G32B32A32_UINT;

    case FORMAT_R32_SINT:
        return VK_FORMAT_R32_SINT;
    case FORMAT_R32G32_SINT:
        return VK_FORMAT_R32G32_SINT;
    case FORMAT_R32G32B32_SINT:
        return VK_FORMAT_R32G32B32_SINT;
    case FORMAT_R32G32B32A32_SINT:
        return VK_FORMAT_R32G32B32A32_SINT;

    case FORMAT_D16_UNORM:
        return VK_FORMAT_D16_UNORM;
    case FORMAT_D24_UNORM_S8_UINT:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case FORMAT_D32_FLOAT:
        return VK_FORMAT_D32_SFLOAT;

    default:
        DRE_ASSERT(false, "VKW::Format not supported.");
        return VK_FORMAT_UNDEFINED;
    }
}

Format VK2Format(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_UNDEFINED:
        return FORMAT_UNDEFINED;

    case VK_FORMAT_R8_UNORM:
        return FORMAT_R8_UNORM;
    case VK_FORMAT_R8G8_UNORM:
        return FORMAT_R8G8_UNORM;
    case VK_FORMAT_R8G8B8A8_UNORM:
        return FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_B8G8R8A8_UNORM:
        return FORMAT_B8G8R8A8_UNORM;

    case VK_FORMAT_R8_SRGB:
        return FORMAT_R8_SRGB;
    case VK_FORMAT_R8G8_SRGB:
        return FORMAT_R8G8_SRGB;
    case VK_FORMAT_R8G8B8A8_SRGB:
        return FORMAT_R8G8B8A8_SRGB;
    case VK_FORMAT_B8G8R8A8_SRGB:
    return FORMAT_B8G8R8A8_SRGB;

    case VK_FORMAT_R16_SFLOAT:
        return FORMAT_R16_FLOAT;
    case VK_FORMAT_R16G16_SFLOAT:
        return FORMAT_R16G16_FLOAT;
    case VK_FORMAT_R16G16B16_SFLOAT:
        return FORMAT_R16G16B16_FLOAT;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return FORMAT_R16G16B16A16_FLOAT;

    case VK_FORMAT_R32_SFLOAT:
        return FORMAT_R32_FLOAT;
    case VK_FORMAT_R32G32_SFLOAT:
        return FORMAT_R32G32_FLOAT;
    case VK_FORMAT_R32G32B32_SFLOAT:
        return FORMAT_R32G32B32_FLOAT;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return FORMAT_R32G32B32A32_FLOAT;

    case VK_FORMAT_R8_UINT:
        return FORMAT_R8_UINT;
    case VK_FORMAT_R8G8_UINT:
        return FORMAT_R8G8_UINT;
    case VK_FORMAT_R8G8B8_UINT:
        return FORMAT_R8G8B8_UINT;
    case VK_FORMAT_R8G8B8A8_UINT:
        return FORMAT_R8G8B8A8_UINT;

    case VK_FORMAT_R8_SINT:
        return FORMAT_R8_SINT;
    case VK_FORMAT_R8G8_SINT:
        return FORMAT_R8G8_SINT;
    case VK_FORMAT_R8G8B8_SINT:
        return FORMAT_R8G8B8_SINT;
    case VK_FORMAT_R8G8B8A8_SINT:
        return FORMAT_R8G8B8A8_SINT;

    case VK_FORMAT_R16_UINT:
        return FORMAT_R16_UINT;
    case VK_FORMAT_R16G16_UINT:
        return FORMAT_R16G16_UINT;
    case VK_FORMAT_R16G16B16_UINT:
        return FORMAT_R16G16B16_UINT;
    case VK_FORMAT_R16G16B16A16_UINT:
        return FORMAT_R16G16B16A16_UINT;

    case VK_FORMAT_R16_SINT:
        return FORMAT_R16_SINT;
    case VK_FORMAT_R16G16_SINT:
        return FORMAT_R16G16_SINT;
    case VK_FORMAT_R16G16B16_SINT:
        return FORMAT_R16G16B16_SINT;
    case VK_FORMAT_R16G16B16A16_SINT:
        return FORMAT_R16G16B16A16_SINT;

    case VK_FORMAT_R32_UINT:
        return FORMAT_R32_UINT;
    case VK_FORMAT_R32G32_UINT:
        return FORMAT_R32G32_UINT;
    case VK_FORMAT_R32G32B32_UINT:
        return FORMAT_R32G32B32_UINT;
    case VK_FORMAT_R32G32B32A32_UINT:
        return FORMAT_R32G32B32A32_UINT;

    case VK_FORMAT_R32_SINT:
        return FORMAT_R32_SINT;
    case VK_FORMAT_R32G32_SINT:
        return FORMAT_R32G32_SINT;
    case VK_FORMAT_R32G32B32_SINT:
        return FORMAT_R32G32B32_SINT;
    case VK_FORMAT_R32G32B32A32_SINT:
        return FORMAT_R32G32B32A32_SINT;

    case VK_FORMAT_D16_UNORM:
        return FORMAT_D16_UNORM;
    case VK_FORMAT_D24_UNORM_S8_UINT:
        return FORMAT_D24_UNORM_S8_UINT;
    case VK_FORMAT_D32_SFLOAT:
        return FORMAT_D32_FLOAT;

    default:
        DRE_ASSERT(false, "VkFormat not supported.");
        return FORMAT_UNDEFINED;
    }
}


}

