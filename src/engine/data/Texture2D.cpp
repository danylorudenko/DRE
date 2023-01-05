#include <data\Texture2D.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb\stb_image.h>

namespace Data
{

bool Texture2D::IsInitialized() const
{
    return textureChannelVariations_ != Data::TEXTURE_VARIATION_INVALID;
}

void Texture2D::ReadFromFile(char const* filePath, TextureChannelVariations channelVariations)
{
    int desiredChannels = 0;
    switch (channelVariations)
    {
    case Data::TEXTURE_VARIATION_GRAY:
        desiredChannels = STBI_grey;
        break;
    case Data::TEXTURE_VARIATION_GRAY_ALPHA:
        desiredChannels = STBI_grey_alpha;
        break;
    case Data::TEXTURE_VARIATION_RGB:
        desiredChannels = STBI_rgb;
        break;
    case Data::TEXTURE_VARIATION_RGBA:
        desiredChannels = STBI_rgb_alpha;
        break;
    default:
        assert(false && "Invalid TextureChannelVariations");
    }

    int x, y, n;
    stbi_uc* stbiData = stbi_load(filePath, &x, &y, &n, desiredChannels);
    if (stbiData == NULL) {
        textureChannelVariations_ = Data::TEXTURE_VARIATION_INVALID;
        filePath_ = nullptr;
        width_ = 0;
        height_ = 0;
        return;
    }

    std::uint32_t const dataSize = x * y * desiredChannels;
    textureData_.Resize(dataSize);
    std::memcpy(textureData_.Data(), stbiData, dataSize);

    textureChannelVariations_ = channelVariations;
    filePath_ = filePath;
    width_ = x;
    height_ = y;

    stbi_image_free(stbiData);
}

TextureChannelVariations Texture2D::GetChannelVariations() const
{
    return textureChannelVariations_;
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

