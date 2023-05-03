#include <engine\data\Texture2D.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Data
{

Texture2D::Texture2D()
    : name_{}
    , format_{ VKW::FORMAT_UNDEFINED }
    , textureData_{}
    , width_{ 0 }
    , height_{ 0 }
{}


bool Texture2D::IsInitialized() const
{
    return format_ != VKW::FORMAT_UNDEFINED;
}

void Texture2D::ReadFromFile(char const* filePath, TextureChannelVariations channelVariations)
{
    int desiredChannels = 0;
    switch (channelVariations)
    {
    case Data::TEXTURE_VARIATION_GRAY:
        desiredChannels = STBI_grey;
        format_ = VKW::FORMAT_R8_UNORM;
        break;
    case Data::TEXTURE_VARIATION_GRAY_ALPHA:
        desiredChannels = STBI_grey_alpha;
        format_ = VKW::FORMAT_R8G8_UNORM;
        break;
    case Data::TEXTURE_VARIATION_RGBA:
        desiredChannels = STBI_rgb_alpha;
        format_ = VKW::FORMAT_R8G8B8A8_UNORM;
        break;
    default:
        assert(false && "Invalid TextureChannelVariations");
    }

    int x, y, n;
    stbi_uc* stbiData = stbi_load(filePath, &x, &y, &n, desiredChannels);
    if (stbiData == NULL) {
        format_ = VKW::FORMAT_UNDEFINED;
        DRE_ASSERT(stbiData != NULL, "stbi failed to load texture.");
        return;
    }

    std::uint32_t const dataSize = x * y * desiredChannels;
    textureData_.Resize(dataSize);
    std::memcpy(textureData_.Data(), stbiData, dataSize);

    name_ = filePath;
    width_ = x;
    height_ = y;

    stbi_image_free(stbiData);
}

VKW::Format Texture2D::GetFormat() const
{
    return format_;
}

DRE::ByteBuffer const& Texture2D::GetBuffer() const
{
    return textureData_;
}

std::uint32_t Texture2D::GetSizeX() const
{
    return width_;
}

std::uint32_t Texture2D::GetSizeY() const
{
    return height_;
}

}

