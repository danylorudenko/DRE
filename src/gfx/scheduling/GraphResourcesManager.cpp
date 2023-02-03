#include <gfx\scheduling\GraphResourcesManager.hpp>

#include <foundation\Common.hpp>

#include <vk_wrapper\Device.hpp>

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

void GraphResourcesManager::RegisterTexture(TextureID id, VKW::Format format, std::uint32_t width, std::uint32_t height, VKW::ResourceAccess access, VKW::Stages stage)
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

void GraphResourcesManager::RegisterBuffer(BufferID id, std::uint32_t size, VKW::ResourceAccess access, VKW::Stages stage)
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

void GraphResourcesManager::PrepareResources()
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
        if (info.access | VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT)
        {
            usage = VKW::ImageUsage::RENDER_TARGET;
        }
        else if (info.access | VKW::RESOURCE_ACCESS_DEPTH_STENCIL_ATTACHMENT)
        {
            usage = VKW::ImageUsage::DEPTH_STENCIL;
        }
 

        VKW::ImageResource* image    = m_Device->GetResourcesController()->CreateImage(info.size0, info.size1, info.format, usage);
        VKW::ImageResourceView* view = m_Device->GetResourcesController()->ViewImageAs(image);

        m_StorageTextures[*pair.key] = GraphTexture{ StorageTexture{ m_Device, image, view }, info };
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
        VKW::BufferResource* buffer = m_Device->GetResourcesController()->CreateBuffer(info.size0, VKW::BufferUsage::STORAGE);
        m_StorageBuffers.Emplace(*pair.key, GraphBuffer{ StorageBuffer{ m_Device, buffer }, info });
        
    });

}

StorageBuffer* GraphResourcesManager::GetStorageBuffer(BufferID id)
{
    return &m_StorageBuffers.Find(id).value->buffer;
}

StorageTexture* GraphResourcesManager::GetStorageTexture(TextureID id)
{
    return &m_StorageTextures.Find(id).value->texture;
}


}

