#pragma once

#include <vulkan\vulkan.h>

#include <foundation\Container\InplaceVector.hpp>
#include <foundation\Container\InplaceHashTable.hpp>

#include <vk_wrapper\pipeline\Dependency.hpp>

namespace VKW
{
class Context;
}

namespace GFX
{

class BasePass;
class TextureBase;
class BufferBase;
class Device;
class GraphResourcesManager;

/////////////////////////////////////
class DependencyManager
    : public NonCopyable
    , public NonMovable
{
public:
    DependencyManager();

    virtual ~DependencyManager();

    void ResourceBarrier(VKW::Context& context, VKW::ImageResource* resource, VKW::ResourceAccess access, VKW::Stages stageFlags);
    void ResourceBarrier(VKW::Context& context, VKW::BufferResource* resource, VKW::ResourceAccess access, VKW::Stages stageFlags);


private:
    struct TextureAccessEntry
    {
        VKW::ResourceAccess access      = VKW::RESOURCE_ACCESS_UNDEFINED;
        VKW::Stages         stage       = VKW::STAGE_UNDEFINED;
    };

    struct BufferAccessEntry
    {
        VKW::ResourceAccess  access;
        VKW::Stages          stage;
    };

    DRE::InplaceHashTable<VKW::ImageResource*,  TextureAccessEntry> m_TextureHistory;
    DRE::InplaceHashTable<VKW::BufferResource*, BufferAccessEntry>  m_BufferHistory;
};

}

