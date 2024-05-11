#include <gfx\scheduling\GraphResourcesManager.hpp>

#include <foundation\Common.hpp>

#include <vk_wrapper\Device.hpp>
#include <vk_wrapper\Helper.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\pass\BasePass.hpp>
#include <gfx\buffer\UniformProxy.hpp>

namespace GFX
{

GraphResourcesManager::GraphResourcesManager(VKW::Device* device)
    : m_Device{ device }
{

}

GraphResourcesManager::~GraphResourcesManager() = default;

void GraphResourcesManager::RegisterTexture(char const* id, VKW::Format format, std::uint32_t width, std::uint32_t height, VKW::ResourceAccess access)
{
    AccumulatedInfo& info = m_AccumulatedTextureInfo[id];

    DRE_DEBUG_ONLY(if (info.access != VKW::RESOURCE_ACCESS_UNDEFINED))
        DRE_ASSERT(info.size0 == width && info.size1 == height && info.depth == 1, "Different sized specified for same resource");

    info.access = VKW::ResourceAccess(info.access | std::uint64_t(access));
    info.format = format;
    info.size0 = width;
    info.size1 = height;
    info.depth = 1;
}

void GraphResourcesManager::RegisterBuffer(char const* id, std::uint32_t size, VKW::ResourceAccess access)
{
    AccumulatedInfo& info = m_AccumulatedBufferInfo[id];

    DRE_DEBUG_ONLY(if (info.access != VKW::RESOURCE_ACCESS_UNDEFINED))
        DRE_ASSERT(info.size0 == size, "Different sized specified for same resource");

    info.access = VKW::ResourceAccess(info.access | std::uint64_t(access));
    info.format = VKW::FORMAT_UNDEFINED;
    info.size0 = size;
    info.size1 = 0;
    info.depth = 0;
}

void GraphResourcesManager::InitResources()
{
    m_AccumulatedTextureInfo.ForEach([this](auto& pair)
    {
        AccumulatedInfo const& info = *pair.value;
        auto texturePair = m_StorageTextures.Find(*pair.key);
        if (texturePair.key != nullptr)
        {
            if (texturePair.value->info == info)
                return;
        }


        VKW::ImageUsage usage = VKW::ImageUsage::STORAGE_IMAGE;
        VkImageAspectFlags imageAspect = VK_IMAGE_ASPECT_COLOR_BIT;
        if (info.access & VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT)
        {
            usage = VKW::ImageUsage::RENDER_TARGET;
            imageAspect = VK_IMAGE_ASPECT_COLOR_BIT;
        }
        else if (info.access & VKW::RESOURCE_ACCESS_DEPTH_STENCIL_ATTACHMENT)
        {
            usage = VKW::ImageUsage::DEPTH_STENCIL;
            imageAspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        else if (info.access & VKW::RESOURCE_ACCESS_DEPTH_ONLY_ATTACHMENT)
        {
            usage = VKW::ImageUsage::DEPTH;
            if ((info.access & (VKW::RESOURCE_ACCESS_SHADER_READ | VKW::RESOURCE_ACCESS_SHADER_WRITE | VKW::RESOURCE_ACCESS_SHADER_RW | VKW::RESOURCE_ACCESS_SHADER_SAMPLE)) != 0)
                usage = VKW::ImageUsage::DEPTH_SAMPLED;
            imageAspect = VK_IMAGE_ASPECT_DEPTH_BIT;
        }
 
        //if (*pair.key == TextureID::FFTHxt)
        //    DebugBreak();

        VKW::ImageResource* image    = m_Device->GetResourcesController()->CreateImage(info.size0, info.size1, info.format, usage, *pair.key);


        VkImageSubresourceRange range = VKW::HELPER::DefaultImageSubresourceRange(imageAspect);
        VKW::ImageResourceView* view = m_Device->GetResourcesController()->ViewImageAs(image, &range);
        VKW::TextureDescriptorIndex globalDescriptor = m_Device->GetDescriptorManager()->AllocateTextureDescriptor(view);

        m_StorageTextures[*pair.key] = GraphTexture{ StorageTexture{ m_Device, image, view, globalDescriptor }, info };
    });
 

    m_AccumulatedBufferInfo.ForEach([this](auto& pair)
    {
        AccumulatedInfo const& info = *pair.value;
        auto bufferPair = m_StorageBuffers.Find(*pair.key);
        if (bufferPair.key != nullptr)
        {
            if (bufferPair.value->info == info)
                return;
        }

        DRE_ASSERT(info.access | VKW::RESOURCE_ACCESS_SHADER_RW, "If there's no shader access, why we need this buffer?");

        // create storage buffer
        VKW::BufferResource* buffer = m_Device->GetResourcesController()->CreateBuffer(info.size0, VKW::BufferUsage::STORAGE, *pair.key);
        m_StorageBuffers.Emplace(*pair.key, GraphBuffer{ StorageBuffer{ m_Device, buffer }, info });
    });
}

void GraphResourcesManager::DestroyResources()
{
    m_StorageBuffers.Clear();
    m_StorageTextures.Clear();
}

StorageBuffer* GraphResourcesManager::GetBuffer(char const* id)
{
    return &m_StorageBuffers.Find(id).value->buffer;
}

StorageTexture* GraphResourcesManager::GetTexture(char const* id)
{
    return &m_StorageTextures.Find(id).value->texture;
}


}

