#include <gfx\buffer\PersistentStorage.hpp>

#include <gfx\GraphicsManager.hpp>

namespace GFX
{

void PersistentStorage::Allocation::Update(VKW::Context& context, void* data, std::uint32_t size)
{
    Update(context, 0, data, size);
}

void PersistentStorage::Allocation::Update(VKW::Context& context, std::uint32_t dstOffset, void* data, std::uint32_t size)
{
    UploadArena::Allocation allocation = m_UploadArena->AllocateTransientRegion(g_GraphicsManager->GetCurrentFrameID(), size, 8);
    std::memcpy(allocation.m_MappedRange, data, size);
    allocation.FlushCaches();

    Update(context, dstOffset, allocation);
}

void PersistentStorage::Allocation::Update(VKW::Context& context, std::uint32_t dstOffset, UploadArena::Allocation const& src)
{
    Update(context, dstOffset, src.m_Buffer, src.m_OffsetInBuffer, src.m_Size);
}

void PersistentStorage::Allocation::Update(VKW::Context& context, std::uint32_t dstOffset, VKW::BufferResource* src, std::uint32_t srcOffset, std::uint32_t srcSize)
{
    DRE_ASSERT(srcSize < GetSize(), "Data is bigger than default persistent storage chunk.");

    context.CmdResourceDependency(src, srcOffset, srcSize,
        VKW::RESOURCE_ACCESS_HOST_WRITE, VKW::STAGE_HOST,
        VKW::RESOURCE_ACCESS_TRANSFER_SRC, VKW::STAGE_TRANSFER);

    VKW::BufferResource* buffer = m_Buffer->GetResource();
    std::uint32_t const storageOffset = m_Offset;
    context.CmdResourceDependency(buffer, storageOffset, srcSize,
        VKW::RESOURCE_ACCESS_NONE, VKW::STAGE_TOP,
        VKW::RESOURCE_ACCESS_TRANSFER_DST, VKW::STAGE_TRANSFER);

    context.CmdCopyBufferToBuffer(buffer, storageOffset + dstOffset, src, srcOffset, srcSize);

    context.CmdResourceDependency(buffer, storageOffset, GetSize(),
        VKW::RESOURCE_ACCESS_TRANSFER_DST, VKW::STAGE_TRANSFER,
        VKW::RESOURCE_ACCESS_SHADER_READ, VKW::STAGE_ALL_GRAPHICS | VKW::STAGE_COMPUTE);
}

std::uint32_t PersistentStorage::Allocation::GetSize() const
{
    return m_Size;
}

std::uint64_t PersistentStorage::Allocation::GetGPUAddress() const
{
    return m_Buffer->GetResource()->gpuAddress_ + m_Offset;
}

PersistentStorage::Allocation::Allocation(StorageBuffer* buffer, UploadArena* uploadArena, std::uint32_t size, std::uint32_t offset)
    : m_Buffer{ buffer }
    , m_UploadArena{ uploadArena }
    , m_Size{ size }
    , m_Offset{ offset }
{
}

PersistentStorage::PersistentStorage(VKW::Device* device, UploadArena* uploadArena, VKW::Device* divice, std::uint32_t size)
    : m_UploadArena{ uploadArena }
    , m_Buffer{}
    , m_Size{ size }
    , m_FreeOffset{ 0 }
{
    VKW::BufferResource* resource = device->GetResourcesController()->CreateBuffer(size, VKW::BufferUsage::STORAGE);
    m_Buffer = StorageBuffer{ device, resource };
}

PersistentStorage::Allocation PersistentStorage::AllocateRegion(std::uint32_t size, std::uint32_t alignment)
{
    std::uint32_t const allocationOffset = DRE::Align(m_FreeOffset, alignment);
    m_FreeOffset = m_FreeOffset + size + alignment;
    DRE_ASSERT(m_FreeOffset <= m_Size, "PersistentStorage is out of space");

    return Allocation{ &m_Buffer, m_UploadArena, size, allocationOffset };
}

}
