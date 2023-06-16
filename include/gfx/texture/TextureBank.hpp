#pragma once

#include <foundation\class_features\NonMovable.hpp>
#include <foundation\class_features\NonCopyable.hpp>

#include <foundation\memory\ByteBuffer.hpp>

#include <foundation\String\InplaceString.hpp>
#include <foundation\Container\InplaceHashTable.hpp>

#include <gfx\texture\ReadOnlyTexture.hpp>

namespace VKW
{
class ResourcesController;
class DescriptorManager;
class Context;
}

namespace GFX
{

class TextureBank
    : public NonCopyable
    , public NonMovable
{
public:
    TextureBank(VKW::Context* loadingContext, VKW::ResourcesController* resourcesController, VKW::DescriptorManager* descriptorAllocator);
    ~TextureBank();

    void                LoadDefaultTextures ();
    ReadOnlyTexture*    LoadTexture2DSync   (DRE::String128 const& name, std::uint32_t width, std::uint32_t height, VKW::Format format, DRE::ByteBuffer const& textureData);
    ReadOnlyTexture*    FindTexture         (DRE::String128 const& name);

private:
    void                GenFFTIndexTexture  (std::uint32_t n);

private:
    VKW::ResourcesController* m_ResourcesController;
    VKW::DescriptorManager*   m_DescriptorAllocator;
    VKW::Context*             m_LoadingContext;

    DRE::InplaceHashTable<DRE::String128, ReadOnlyTexture> m_DiscTextures;
};

}
