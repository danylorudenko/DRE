#pragma once

#include <foundation\memory\ByteBuffer.hpp>

namespace Data
{

enum TextureChannelVariations : std::uint32_t
{
    TEXTURE_VARIATION_INVALID,
    TEXTURE_VARIATION_GRAY,
    TEXTURE_VARIATION_GRAY_ALPHA,
    TEXTURE_VARIATION_RGB,
    TEXTURE_VARIATION_RGBA
};

class Texture2D
{
public:
    bool IsInitialized() const;
    void ReadFromFile(char const* filePath, TextureChannelVariations channelVariations);

    TextureChannelVariations GetChannelVariations() const;
    DRE::ByteBuffer const& GetBuffer() const;
    std::uint32_t GetSizeX() const;
    std::uint32_t GetSizeY() const;


private:
    TextureChannelVariations textureChannelVariations_;
    DRE::ByteBuffer textureData_;
    char const* filePath_;

    std::uint32_t width_;
    std::uint32_t height_;
};

}