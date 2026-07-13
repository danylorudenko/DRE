#pragma once

#include <foundation\Container\InplaceHashTable.hpp>
#include <foundation\string\InplaceString.hpp>

#include <vk_wrapper\Format.hpp>
#include <vk_wrapper\pipeline\Dependency.hpp>

#include <gfx\scheduling\GraphResource.hpp>

#include <gfx\buffer\StorageBuffer.hpp>
#include <gfx\buffer\TransientArena.hpp>
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
        VKW::ResourceAccess access  = VKW::RESOURCE_ACCESS_INVALID;
        VKW::Format         format  = VKW::FORMAT_UNDEFINED;
        DRE::U32            size0   = 0;
        DRE::U32            size1   = 0;
        DRE::U32            mipCount   = 0;
        DRE::U32            depth   = 0;
        DRE::U32            flags = GraphResourceFlags::NONE;

        inline bool operator==(AccumulatedInfo const& rhs)
        {
            return 
                access == rhs.access &&
                format == rhs.format &&
                size0  == rhs.size0 &&
                size1  == rhs.size1 &&
                mipCount == rhs.mipCount &&
                depth  == rhs.depth &&
                flags  == rhs.flags;
        }
    };

    struct GraphBuffer
    {
        DRE::InplaceVector<StorageBuffer, 2> temporalStorage;
        AccumulatedInfo info;
    };

    struct GraphTexture
    {
        DRE::InplaceVector<Texture, 2> temporalStorage;
        AccumulatedInfo info;
    };


public:
    GraphResourcesManager(VKW::Device* device);

    virtual ~GraphResourcesManager();

    void RegisterTexture(char const* id, VKW::Format format, DRE::U32 width, DRE::U32 height, VKW::ResourceAccess access, GraphResourceFlags flags = GraphResourceFlags::NONE);
    void RegisterTexture(char const* id, VKW::Format format, DRE::U32 width, DRE::U32 height, DRE::U32 mipCount, VKW::ResourceAccess access, GraphResourceFlags flags = GraphResourceFlags::NONE);
    void RegisterBuffer(char const* id, DRE::U32 size, VKW::ResourceAccess access, GraphResourceFlags flags = GraphResourceFlags::NONE);

    void InitResources();
    void DestroyResources();

    StorageBuffer*  GetBuffer    (char const* id);
    Texture*        GetTexture   (char const* id);

    StorageBuffer*  GetTemporalBuffer(char const* id, FrameID frameID);
    Texture*        GetTemporalTexture(char const* id, FrameID frameID);

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


