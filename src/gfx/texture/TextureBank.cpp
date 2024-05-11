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
    m_Textures.Clear();
}

ReadOnlyTexture* TextureBank::FindTexture(DRE::String128 const& name)
{
    return m_Textures.Find(name).value;
}

void TextureBank::LoadDefaultTextures()
{
    DRE::U8 defaultColor[4] = { 0x00, 0x00, 0x00, 0x00 };
    DRE::U8 defaultNormal[4] = { 0x00, 0x00, 0xFF, 0x00 };
    DRE::U8 zero = 0x00;
    DRE::U8 one = 0xFF;
    DRE::ByteBuffer defaultColorBuffer{ defaultColor, sizeof(defaultColor) };
    DRE::ByteBuffer defaultNormalBuffer{ defaultNormal, sizeof(defaultNormal) };
    DRE::ByteBuffer zeroBuffer{ &zero, sizeof(zero) };
    DRE::ByteBuffer oneBuffer{ &one, sizeof(one) };

    LoadTexture2DSync("default_color", 1, 1, VKW::FORMAT_R8G8B8A8_UNORM, defaultColorBuffer);
    LoadTexture2DSync("default_normal", 1, 1, VKW::FORMAT_R8G8B8A8_UNORM, defaultNormalBuffer);
    LoadTexture2DSync("zero_r", 1, 1, VKW::FORMAT_R8_UNORM, zeroBuffer);
    LoadTexture2DSync("one_r", 1, 1, VKW::FORMAT_R8_UNORM, oneBuffer);
}

struct Complex
{
    float r;
    float i;

    Complex operator+(Complex const& rhs)
    {
        return Complex{ rhs.r, rhs.i };
    }

    Complex operator*(Complex const& rhs)
    {
        return Complex{ r * rhs.r - i * rhs.i, r * rhs.i + rhs.r * i };
    }
};

ReadOnlyTexture* TextureBank::LoadTexture2DSync(DRE::String128 const& name, std::uint32_t width, std::uint32_t height, VKW::Format format, DRE::ByteBuffer const& textureData)
{
    UploadArena& transientArena = g_GraphicsManager->GetUploadArena();

    // 1. staging buffer region and target texture
    VKW::ImageResource* imageResource = m_ResourcesController->CreateImage(width, height, format, VKW::ImageUsage::TEXTURE);
    UploadArena::Allocation stagingRegion = transientArena.AllocateTransientRegion(g_GraphicsManager->GetCurrentFrameID(), static_cast<std::uint32_t>(textureData.Size()), 16);
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
        VKW::RESOURCE_ACCESS_TRANSFER_DST,  VKW::STAGE_TRANSFER,
        VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_ALL_GRAPHICS | VKW::STAGE_COMPUTE);

    VKW::QueueExecutionPoint syncPoint = m_LoadingContext->SyncPoint();
    m_LoadingContext->FlushAll();
    syncPoint.Wait();
    transientArena.ResetAllocations(g_GraphicsManager->GetCurrentFrameID());

    VKW::ImageResourceView* imageView = m_ResourcesController->ViewImageAs(imageResource);
    VKW::TextureDescriptorIndex descriptorHandle = m_DescriptorAllocator->AllocateTextureDescriptor(imageView);

    return &(m_Textures[name] = ReadOnlyTexture{ g_GraphicsManager->GetMainDevice(), imageResource, imageView, descriptorHandle });
}

void TextureBank::UnloadAllTextures()
{
    m_Textures.Clear();
}

void TextureBank::GenFFTIndexTexture(std::uint32_t n)
{
    /*
    DRE_ASSERT(DRE::IsPowOf2(n), "FFT sample count must be PowOf2!");

    UploadArena& transientArena = g_GraphicsManager->GetUploadArena();
    //auto allocation = transientArena.AllocateTransientRegion().

    std::uint32_t const height = glm::log2(float(n));

    for (std::uint32_t row = 0; row < height; row++)
    {
        for (std::uint32_t column = 0; column < n; column++)
        {
            std::uint32_t k = ((row * n) / (glm::pow(2, column + 1)));
            k %= n;

            bool topOp = glm::pow(2, column + 1) < glm::pow(2, column);


        }
    }

    */
}

}