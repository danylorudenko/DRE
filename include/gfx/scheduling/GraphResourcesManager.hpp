#pragma once

#include <foundation\Container\InplaceHashTable.hpp>
#include <foundation\string\InplaceString.hpp>

#include <vk_wrapper\Format.hpp>
#include <vk_wrapper\pipeline\Dependency.hpp>

#include <gfx\pass\PassID.hpp>
#include <gfx\scheduling\GraphResource.hpp>

#include <gfx\buffer\StorageBuffer.hpp>
#include <gfx\buffer\TransientArena.hpp>
#include <gfx\buffer\UniformProxy.hpp>
#include <gfx\texture\Texture.hpp>

namespace GFX
{

//////////////////////////////////////
class GraphResourcesManager
    : public NonCopyable
    , public NonMovable
{
public:
    struct AccumulatedInfo
    {
        VKW::ResourceAccess access  = VKW::RESOURCE_ACCESS_UNDEFINED;
        VKW::Format         format  = VKW::FORMAT_UNDEFINED;
        DRE::U32            size0   = 0;
        DRE::U32            size1   = 0;
        DRE::U32            mipCount   = 0;
        DRE::U32            depth   = 0;

        inline bool operator==(AccumulatedInfo const& rhs)
        {
            return 
                access == rhs.access &&
                format == rhs.format &&
                size0  == rhs.size0 &&
                size1  == rhs.size1 &&
                depth  == rhs.depth;
        }
    };

    struct GraphBuffer
    {
        StorageBuffer   buffer;
        AccumulatedInfo info;
    };

    struct GraphTexture
    {
        Texture         texture;
        AccumulatedInfo info;
    };


public:
    GraphResourcesManager(VKW::Device* device);

    virtual ~GraphResourcesManager();

    void RegisterTexture(char const* id, VKW::Format format, DRE::U32 width, DRE::U32 height, VKW::ResourceAccess access);
    void RegisterTexture(char const* id, VKW::Format format, DRE::U32 width, DRE::U32 height, DRE::U32 mipCount, VKW::ResourceAccess access);
    void RegisterBuffer(char const* id, DRE::U32 size, VKW::ResourceAccess access);

    void InitResources();
    void DestroyResources();

    StorageBuffer*  GetBuffer    (char const* id);
    Texture*        GetTexture   (char const* id);

    template<typename TDelegate>
    void ForEachTexture(TDelegate func)
    {
        m_StorageTextures.ForEach(func);
    }

private:
    VKW::Device*        m_Device;

    DRE::InplaceHashTable<DRE::String64, GraphBuffer>  m_StorageBuffers;
    DRE::InplaceHashTable<DRE::String64, GraphTexture> m_StorageTextures;

    DRE::InplaceHashTable<DRE::String64, AccumulatedInfo> m_AccumulatedBufferInfo;
    DRE::InplaceHashTable<DRE::String64, AccumulatedInfo> m_AccumulatedTextureInfo;
};

}


