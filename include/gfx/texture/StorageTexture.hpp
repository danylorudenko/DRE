#pragma once

#include <foundation\Container\InplaceVector.hpp>
#include <vk_wrapper\Constant.hpp>
#include <vk_wrapper\descriptor\DescriptorManager.hpp>
#include <gfx\texture\TextureBase.hpp>

namespace VKW
{
struct ImageResource;
struct ImageResourceView;
}

namespace GFX
{

class Context;

class StorageTexture
    : public TextureBase
{
public:
    StorageTexture();
    StorageTexture(VKW::Device* device, VKW::ImageResource* image, VKW::ImageResourceView* view);

    StorageTexture(StorageTexture&& rhs);
    StorageTexture& operator=(StorageTexture&& rhs);

    virtual ~StorageTexture();

    inline VKW::ImageResourceView* GetShaderView() const { return m_ShaderView; }

private:
    VKW::ImageResourceView* m_ShaderView;
};

}

