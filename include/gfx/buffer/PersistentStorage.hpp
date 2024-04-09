#pragma once

#include <foundation\class_features\NonMovable.hpp>
#include <foundation\class_features\NonCopyable.hpp>

#include <gfx\buffer\StorageBuffer.hpp>
#include <gfx\buffer\TransientArena.hpp>

namespace VKW
{
class Device;
class Context;
}

namespace GFX
{

class PersistentStorage
{
public:
    class Allocation
    {
        friend class PersistentStorage;

    public:
        ~Allocation() = default;

        template<typename T>
        void Update(VKW::Context& context, T& data)
        {
            Update(context, &data, sizeof(data));
        }

        void Update(VKW::Context& context, void* data, std::uint32_t size);
        void Update(VKW::Context& context, UploadArena::Allocation const& src);
        void Update(VKW::Context& context, VKW::BufferResource* src, std::uint32_t srcOffset, std::uint32_t srcSize);
        void Update(VKW::Context& context, std::uint32_t dstOffset, VKW::BufferResource* src, std::uint32_t srcOffset, std::uint32_t srcSize);

        std::uint32_t GetSize() const;
        std::uint64_t GetGPUAddress() const;

    private:
        Allocation(StorageBuffer* buffer, UploadArena* uploadArena, std::uint32_t size, std::uint32_t offset);

    private:
        StorageBuffer*          m_Buffer;
        UploadArena*            m_UploadArena;
        std::uint32_t           m_Size;
        std::uint32_t           m_Offset;
    };

public:
    PersistentStorage(VKW::Device* device, UploadArena* uploadArena, VKW::Device* divice, std::uint32_t size);

    ~PersistentStorage() = default;

    StorageBuffer* GetStorage() { return &m_Buffer; }

    Allocation AllocateRegion(std::uint32_t size, std::uint32_t alignment = 16u);

    template<typename T>
    Allocation AllocateRegion()
    {
        return AllocateRegion(sizeof(T), DRE::Min(alignof(T), 16));
    }

private:
    UploadArena*                                m_UploadArena;
    StorageBuffer                               m_Buffer;
    std::uint32_t                               m_FreeOffset;
    std::uint32_t                               m_Size;
};

}
