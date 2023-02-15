#pragma once

#include <foundation\memory\ByteBuffer.hpp>
#include <foundation\string\InplaceString.hpp>

#include <vk_wrapper\Format.hpp>

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
    Texture2D();

    bool IsInitialized() const;
    void ReadFromFile(char const* filePath, TextureChannelVariations channelVariations);

    VKW::Format GetFormat() const;
    DRE::ByteBuffer const& GetBuffer() const;
    std::uint32_t GetSizeX() const;
    std::uint32_t GetSizeY() const;

    char const* GetName() const { return static_cast<char const*>(name_); }

private:
    DRE::String128 name_;

    VKW::Format format_;
    DRE::ByteBuffer textureData_;

    std::uint32_t width_;
    std::uint32_t height_;
};

}