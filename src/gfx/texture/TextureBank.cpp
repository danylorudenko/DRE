#include <gfx\texture\TextureBank.hpp>

#include <vk_wrapper\Helper.hpp>
#include <vk_wrapper\pipeline\Dependency.hpp>

#include <gfx\GraphicsManager.hpp>

namespace GFX
{

TextureBank::TextureBank(VKW::Context* loadingContext, VKW::ResourcesController* resourcesController, VKW::DescriptorManager* descriptorAllocator)
    : m_LoadingContext{ loadingContext }
    , m_ResourcesController{ resourcesController }
    , m_DescriptorAllocator{ descriptorAllocator }
{

}

TextureBank::~TextureBank()
{

}

ReadOnlyTexture* TextureBank::FindTexture(DRE::String128 const& name)
{
    return m_DiscTextures.Find(name).value;
}

ReadOnlyTexture* TextureBank::LoadTexture2DSync(DRE::String128 const& name, std::uint32_t width, std::uint32_t height, VKW::Format format, DRE::ByteBuffer const& textureData)
{
    UploadArena& transientArena = g_GraphicsManager->GetUploadArena();
    std::uint32_t const loadingQueueFamily = m_LoadingContext->GetParentQueue()->GetQueueFamily();

    // 1. staging buffer region and target texture
    VKW::ImageResource* imageResource = m_ResourcesController->CreateImage(width, height, format, VKW::ImageUsage::TEXTURE);
    UploadArena::Allocation stagingRegion = transientArena.AllocateTransientRegion(g_GraphicsManager->GetCurrentFrameID(), static_cast<std::uint32_t>(textureData.Size()), 4);
    std::memcpy(stagingRegion.m_MappedRange, textureData.Data(), textureData.Size());
    stagingRegion.FlushCaches();

    m_LoadingContext->CmdResourceDependency(imageResource, 
        VKW::RESOURCE_ACCESS_UNDEFINED,     VKW::STAGE_UNDEFINED, 
        VKW::RESOURCE_ACCESS_TRANSFER_DST,  VKW::STAGE_TRANSFER);

    m_LoadingContext->CmdResourceDependency(stagingRegion.m_Buffer, stagingRegion.m_OffsetInBuffer, stagingRegion.m_Size,
        VKW::RESOURCE_ACCESS_HOST_WRITE,    VKW::STAGE_HOST,
        VKW::RESOURCE_ACCESS_TRANSFER_SRC,  VKW::STAGE_TRANSFER);

    m_LoadingContext->CmdCopyBufferToImage(imageResource, stagingRegion.m_Buffer, stagingRegion.m_OffsetInBuffer);

    m_LoadingContext->CmdResourceDependency(imageResource,
        VKW::RESOURCE_ACCESS_TRANSFER_DST, VKW::STAGE_TRANSFER,
        VKW::RESOURCE_ACCESS_NONE, VKW::STAGE_TOP);

    VKW::QueueExecutionPoint syncPoint = m_LoadingContext->SyncPoint();
    m_LoadingContext->FlushOnlyPending();
    syncPoint.Wait();
    transientArena.ResetAllocations(g_GraphicsManager->GetCurrentFrameID());

    VKW::ImageResourceView* imageView = m_ResourcesController->ViewImageAs(imageResource);
    VKW::GlobalDescriptorHandle descriptorHandle = m_DescriptorAllocator->AllocateTextureDescriptor(imageView);

    return &(m_DiscTextures[name] = ReadOnlyTexture{ g_GraphicsManager->GetMainDevice(), imageResource, imageView, descriptorHandle});
}

}