#pragma once

#include <foundation\memory\ByteBuffer.hpp>
#include <foundation\string\InplaceString.hpp>

#include <vk_wrapper\Format.hpp>

namespace Data
{

enum TextureChannels : std::uint32_t
{
    TEXTURE_CHANNELS_INVALID,
    TEXTURE_CHANNELS_GRAY,
    TEXTURE_CHANNELS_GRAY_ALPHA,
    TEXTURE_CHANNELS_RGBA
};

class Texture2D
{
public:
    Texture2D();

    bool IsInitialized() const;
    void ReadFromFile(char const* filePath, TextureChannels channelVariations);

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