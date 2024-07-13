#pragma once

#include <foundation\class_features\NonMovable.hpp>
#include <foundation\class_features\NonCopyable.hpp>

#include <foundation\memory\ByteBuffer.hpp>

#include <foundation\String\InplaceString.hpp>
#include <foundation\Container\InplaceHashTable.hpp>

#include <gfx\texture\Texture.hpp>

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
    Texture*            LoadTexture2DSync   (DRE::String128 const& name, std::uint32_t width, std::uint32_t height, VKW::Format format, DRE::ByteBuffer const& textureData);
    Texture*            FindTexture         (DRE::String128 const& name);

    template<typename TDelegate>
    void                ForEachTexture(TDelegate func) { m_Textures.ForEach(func); }

    void                UnloadAllTextures   ();

private:
    void                GenFFTIndexTexture  (std::uint32_t n);

private:
    VKW::ResourcesController* m_ResourcesController;
    VKW::DescriptorManager*   m_DescriptorAllocator;
    VKW::Context*             m_LoadingContext;

    DRE::InplaceHashTable<DRE::String128, Texture> m_Textures;
};

}
