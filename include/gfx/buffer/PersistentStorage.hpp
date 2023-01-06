#pragma once

#include <foundation\class_features\NonMovable.hpp>
#include <foundation\class_features\NonCopyable.hpp>

#include <foundation\memory\ElementAllocator.hpp>

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
    public:
        Allocation(PersistentStorage* parent, std::uint16_t element);
        ~Allocation();

        template<typename T>
        void Update(VKW::Context& context, VKW::ResourceAccess nextAccess, UploadArena& arena, T& data)
        {
            Update(context, nextAccess, arena, &data, sizeof(data));
        }

        void Update(VKW::Context& context, UploadArena& arena, void* data, std::uint32_t size);
        void Update(VKW::Context& context, UploadArena::Allocation const& src);
        void Update(VKW::Context& context, VKW::BufferResource* src, std::uint32_t srcOffset, std::uint32_t srcSize);

        static constexpr std::uint32_t GetSize() { return C_SIZE; }

    private:
        PersistentStorage*     m_Allocator;
        std::uint16_t          m_Element;

        static constexpr std::uint32_t C_SIZE = 512;
    };

public:
    PersistentStorage(VKW::Device* divice);
    ~PersistentStorage();

    Allocation  AllocateRegion();
    void        FreeRegion(Allocation& region);

    inline StorageBuffer&  GetStorage() { return m_Buffer; }


private:
    VKW::Device*    m_Device;

    StorageBuffer                           m_Buffer;
    DRE::FreeListElementAllocator<128>      m_Allocator;
};

}
