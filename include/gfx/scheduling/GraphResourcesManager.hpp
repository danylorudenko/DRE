#pragma once

#include <foundation\Container\InplaceHashTable.hpp>

#include <vk_wrapper\Format.hpp>
#include <vk_wrapper\pipeline\Dependency.hpp>

#include <gfx\pass\PassID.hpp>
#include <gfx\scheduling\GraphResource.hpp>

#include <gfx\buffer\StorageBuffer.hpp>
#include <gfx\buffer\TransientArena.hpp>
#include <gfx\buffer\UniformProxy.hpp>
#include <gfx\texture\StorageTexture.hpp>

namespace GFX
{

//////////////////////////////////////
class GraphResourcesManager
    : public NonCopyable
    , public NonMovable
{
public:
    GraphResourcesManager(VKW::Device* device);

    virtual ~GraphResourcesManager();

    void RegisterTexture(TextureID id, VKW::Format format, std::uint32_t width, std::uint32_t height, VKW::ResourceAccess access);
    void RegisterBuffer(BufferID id, std::uint32_t size, VKW::ResourceAccess access);

    void PrepareResources();

    StorageBuffer*  GetBuffer    (BufferID id);
    StorageTexture* GetTexture   (TextureID id);


private:
    struct AccumulatedInfo
    {
        VKW::ResourceAccess access  = VKW::RESOURCE_ACCESS_UNDEFINED;
        VKW::Format         format  = VKW::FORMAT_UNDEFINED;
        std::uint32_t       size0   = 0;
        std::uint32_t       size1   = 0;
        std::uint32_t       depth   = 0;

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
        StorageTexture  texture;
        AccumulatedInfo info;
    };

    VKW::Device*        m_Device;

    DRE::InplaceHashTable<BufferID,  GraphBuffer>  m_StorageBuffers;
    DRE::InplaceHashTable<TextureID, GraphTexture> m_StorageTextures;

    DRE::InplaceHashTable<BufferID,  AccumulatedInfo> m_AccumulatedBufferInfo;
    DRE::InplaceHashTable<TextureID, AccumulatedInfo> m_AccumulatedTextureInfo;
};

}


