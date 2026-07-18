#include <gfx\scheduling\GraphResourcesManager.hpp>

#include <foundation\Common.hpp>

#include <vk_wrapper\Device.hpp>
#include <vk_wrapper\Helper.hpp>

#include <gfx\GraphicsManager.hpp>

namespace GFX
{

GraphResourcesManager::GraphResourcesManager(VKW::Device* device)
    : m_Device{ device }
{

}

GraphResourcesManager::~GraphResourcesManager() = default;

void GraphResourcesManager::RegisterTexture(char const* id, VKW::Format format, DRE::U32 width, DRE::U32 height, VKW::ResourceAccess access, DRE::U32 flags)
{
    RegisterTexture(id, format, width, height, 1, access, flags);
}

void GraphResourcesManager::RegisterTexture(char const* id, VKW::Format format, DRE::U32 width, DRE::U32 height, DRE::U32 mipCount, VKW::ResourceAccess access, DRE::U32 flags)
{
    AccumulatedInfo& info = m_AccumulatedTextureInfo[id];

    if (info.access != VKW::RESOURCE_ACCESS_INVALID)
    {
        DRE_ASSERT(info.size0 == width && info.size1 == height && info.depth == 1 && info.mipCount == mipCount && info.format == format && info.flags == flags, "Different sizes specified for same resource");
    }
    else
    {
        // first time init
        info.access = VKW::RESOURCE_ACCESS_NONE;
    }

    info.access = VKW::ResourceAccess(info.access | std::uint64_t(access));
    info.format = format;
    info.size0 = width;
    info.size1 = height;
    info.mipCount = mipCount;
    info.depth = 1;
    info.flags = flags;
}

void GraphResourcesManager::RegisterBuffer(char const* id, DRE::U32 size, VKW::ResourceAccess access, DRE::U32 flags)
{
    AccumulatedInfo& info = m_AccumulatedBufferInfo[id];

    if (info.access != VKW::RESOURCE_ACCESS_INVALID)
    {
        DRE_ASSERT(info.size0 == size && info.size1 == 0 && info.depth == 0 && info.mipCount == 1 && info.format == VKW::FORMAT_UNDEFINED && info.flags == flags, "Different sizes specified for same resource");
    }
    else
    {
        // first time init
        info.access = VKW::RESOURCE_ACCESS_NONE;
    }

    info.access = VKW::ResourceAccess(info.access | std::uint64_t(access));
    info.format = VKW::FORMAT_UNDEFINED;
    info.size0 = size;
    info.size1 = 0;
    info.depth = 0;
    info.mipCount = 1;
    info.flags = flags;
}

void GraphResourcesManager::CreateResources(VKW::Context& context)
{
    m_AccumulatedTextureInfo.ForEach([this, &context](auto& pair)
    {
        AccumulatedInfo const& info = *pair.value;
        auto texturePair = m_StorageTextures.Find(*pair.key);
        if (texturePair.key != nullptr)
        {
            if (texturePair.value->info == info)
                return;
        }


        VKW::ImageUsage usage = VKW::ImageUsage::STORAGE_IMAGE;
        VkImageAspectFlags imageAspect = VKW::Format2Aspect(info.format);
        if (info.access & VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT)
        {
            usage = VKW::ImageUsage::RENDER_TARGET;
            imageAspect |= VK_IMAGE_ASPECT_COLOR_BIT;
        }
        else if (info.access & VKW::RESOURCE_ACCESS_DEPTH_STENCIL_ATTACHMENT)
        {
            usage = VKW::ImageUsage::DEPTH_STENCIL;
            imageAspect |= VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        else if (info.access & VKW::RESOURCE_ACCESS_DEPTH_ONLY_ATTACHMENT)
        {
            usage = VKW::ImageUsage::DEPTH;
            if ((info.access & (VKW::RESOURCE_ACCESS_SHADER_READ | VKW::RESOURCE_ACCESS_SHADER_WRITE | VKW::RESOURCE_ACCESS_SHADER_RW | VKW::RESOURCE_ACCESS_SHADER_SAMPLE)) != 0)
                usage = VKW::ImageUsage::DEPTH_SAMPLED;
            imageAspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
        }

        // create texture
        GraphTexture& graphTexture = m_StorageTextures.Emplace(*pair.key);
        graphTexture.info = info;
        graphTexture.id = *pair.key;

        DRE::String64 subkey0 = *pair.key;
        if (info.flags & GraphResourceFlags::TEMPORAL)
        {
            subkey0.Append("_0");
        }

        VKW::ImageResource* image = m_Device->GetResourcesController()->CreateImage(info.size0, info.size1, info.mipCount, info.format, usage, subkey0);

        VkImageSubresourceRange range = VKW::HELPER::ImageSubresourceRange(imageAspect, image->mipLevels_);
        VKW::ImageResourceView* view = m_Device->GetResourcesController()->ViewImageAs(image, &range);
        VKW::TextureDescriptorIndex globalDescriptor = m_Device->GetDescriptorManager()->AllocateTextureDescriptor(view);

        Texture& texture0 = graphTexture.temporalStorage.EmplaceBack(Texture{ m_Device, image, view, globalDescriptor });
        InitResource(context, info, texture0);

        // temporal brother
        if (info.flags & GraphResourceFlags::TEMPORAL)
        {
            DRE::String64 subkey1 = *pair.key;
            subkey1.Append("_1");

            VKW::ImageResource* temporalImage = m_Device->GetResourcesController()->CreateImage(info.size0, info.size1, info.mipCount, info.format, usage, subkey1);

            VkImageSubresourceRange temporalRange = VKW::HELPER::ImageSubresourceRange(imageAspect, temporalImage->mipLevels_);
            VKW::ImageResourceView* temporalView = m_Device->GetResourcesController()->ViewImageAs(temporalImage, &temporalRange);
            VKW::TextureDescriptorIndex temporalDescriptor = m_Device->GetDescriptorManager()->AllocateTextureDescriptor(temporalView);

            Texture& texture1 = graphTexture.temporalStorage.EmplaceBack(Texture{ m_Device, temporalImage, temporalView, temporalDescriptor });
            InitResource(context, info, texture1);
        }
    });
 

    m_AccumulatedBufferInfo.ForEach([this, &context](auto& pair)
    {
        AccumulatedInfo const& info = *pair.value;
        auto bufferPair = m_StorageBuffers.Find(*pair.key);
        if (bufferPair.key != nullptr)
        {
            if (bufferPair.value->info == info)
                return;
        }

        DRE_ASSERT(info.access | VKW::RESOURCE_ACCESS_SHADER_RW, "If there's no shader access, why we need this buffer?");

        VKW::BufferUsage usage = VKW::BufferUsage::STORAGE;
        if (info.access & VKW::RESOURCE_ACCESS_INDIRECT_ARGS)
            usage = VKW::BufferUsage::INDIRECT_ARGS;

        // create buffer
        GraphBuffer& graphBuffer = m_StorageBuffers.Emplace(*pair.key);
        graphBuffer.info = info;
        graphBuffer.id = *pair.key;

        DRE::String64 subkey0 = *pair.key;
        if (info.flags & GraphResourceFlags::TEMPORAL)
        {
            subkey0.Append("_0");
        }

        VKW::BufferResource* buffer = m_Device->GetResourcesController()->CreateBuffer(info.size0, usage, subkey0);
        StorageBuffer& storageBuffer0 = graphBuffer.temporalStorage.EmplaceBack(StorageBuffer{ m_Device, buffer });
        InitResource(context, info, storageBuffer0);

        // temporal brother
        if (info.flags & GraphResourceFlags::TEMPORAL)
        {
            DRE::String64 subkey1 = *pair.key;
            subkey1.Append("_1");
            VKW::BufferResource* temporalBuffer = m_Device->GetResourcesController()->CreateBuffer(info.size0, usage, subkey1);
            StorageBuffer& storageBuffer1 = graphBuffer.temporalStorage.EmplaceBack(StorageBuffer{ m_Device, temporalBuffer });
            InitResource(context, info, storageBuffer1);
        }
    });
}

void GraphResourcesManager::InitResource(VKW::Context& context, AccumulatedInfo const& info, GFX::Texture& texture)
{
    if (info.flags & GraphResourceFlags::INIT_CLEAR)
    {
        g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, texture.GetResource(), VKW::RESOURCE_ACCESS_TRANSFER_DST, VKW::STAGE_TRANSFER);

        float clearColor[4] = { 0.f, 0.f, 0.f, 0.f };
        context.CmdClearColorImage(texture.GetResource(), clearColor);
    }
}

void GraphResourcesManager::InitResource(VKW::Context& context, AccumulatedInfo const& info, GFX::StorageBuffer& buffer)
{
    if (info.flags & GraphResourceFlags::INIT_CLEAR)
    {
        g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, buffer.GetResource(), VKW::RESOURCE_ACCESS_TRANSFER_DST, VKW::STAGE_TRANSFER);
        context.CmdFillBuffer(buffer.GetResource(), 0, info.size0, 0);
    }
}

void GraphResourcesManager::DestroyResources()
{
    m_StorageBuffers.Clear();
    m_StorageTextures.Clear();
}

StorageBuffer* GraphResourcesManager::GetBuffer(char const* id)
{
    GraphBuffer* resource = m_StorageBuffers.Find(id).value;
    DRE_ASSERT((resource->info.flags & GraphResourceFlags::TEMPORAL) == 0, "Attempt to access buffer via regular accessor with temporal flag.");
    return &resource->temporalStorage[0];
}

Texture* GraphResourcesManager::GetTexture(char const* id)
{
    GraphTexture* resource = m_StorageTextures.Find(id).value;
    DRE_ASSERT((resource->info.flags & GraphResourceFlags::TEMPORAL) == 0, "Attempt to access texture via regular accessor with temporal flag.");
    return &resource->temporalStorage[0];
}

StorageBuffer* GraphResourcesManager::GetTemporalBuffer(char const* id, FrameID frameID)
{
    GraphBuffer* resource = m_StorageBuffers.Find(id).value;
    DRE_ASSERT((resource->info.flags & GraphResourceFlags::TEMPORAL) != 0, "Attempt to access buffer via temporal accessor with no temporal flag.");
    return &resource->temporalStorage[frameID];
}

Texture* GraphResourcesManager::GetTemporalTexture(char const* id, FrameID frameID)
{
    GraphTexture* resource = m_StorageTextures.Find(id).value;
    DRE_ASSERT((resource->info.flags & GraphResourceFlags::TEMPORAL) != 0, "Attempt to access texture via temporal accessor with no temporal flag.");
    return &resource->temporalStorage[frameID];
}

GraphResourcesManager::AccumulatedInfo const* GraphResourcesManager::GetAccumulatedBufferInfo(char const* id)
{
    auto const& pair = m_AccumulatedBufferInfo.Find(id);
    return pair.value;
}

GraphResourcesManager::AccumulatedInfo const* GraphResourcesManager::GetAccumulatedTextureInfo(char const* id)
{
    auto const& pair = m_AccumulatedTextureInfo.Find(id);
    return pair.value;
}

}

