#include <gfx\buffer\PersistentStorage.hpp>

#include <gfx\GraphicsManager.hpp>

namespace GFX
{

PersistentStorage::PersistentStorage(VKW::Device* device)
    : m_Device{ device }
    , m_Buffer{}
    , m_Allocator{}
{
    std::uint32_t constexpr C_BUFFER_SIZE = 128 * 512;

    VKW::BufferResource* resource = device->GetResourcesController()->CreateBuffer(C_BUFFER_SIZE, VKW::BufferUsage::STORAGE);
    m_Buffer = StorageBuffer{ device, resource };
}

PersistentStorage::~PersistentStorage()
{
}

PersistentStorage::Allocation::Allocation(PersistentStorage* parent, std::uint16_t element)
    : m_Allocator{ parent }
    , m_Element{ element }
{
}

PersistentStorage::Allocation::~Allocation()
{
}

void PersistentStorage::Allocation::Update(VKW::Context& context, UploadArena& arena, void* data, std::uint32_t size)
{
    UploadArena::Allocation allocation = arena.AllocateTransientRegion(g_GraphicsManager->GetCurrentFrameID(), size, 8);
    std::memcpy(allocation.m_MappedRange, data, size);
    allocation.FlushCaches();

    Update(context, allocation);
}

void PersistentStorage::Allocation::Update(VKW::Context& context, UploadArena::Allocation const& src)
{
    Update(context, src.m_Buffer, src.m_OffsetInBuffer, src.m_Size);
}

void PersistentStorage::Allocation::Update(VKW::Context& context, VKW::BufferResource* src, std::uint32_t srcOffset, std::uint32_t srcSize)
{
    DRE_ASSERT(srcSize <= GetSize(), "Data is bigger than default persistent storage chunk.");

    context.CmdResourceDependency(src, srcOffset, srcSize,
        VKW::RESOURCE_ACCESS_HOST_WRITE, VKW::STAGE_HOST,
        VKW::RESOURCE_ACCESS_TRANSFER_SRC, VKW::STAGE_TRANSFER);

    StorageBuffer& storage = m_Allocator->GetStorage();
    VKW::BufferResource* resource = storage.GetResource();

    std::uint32_t const storageOffset = GetSize() * m_Element;
    context.CmdResourceDependency(resource, storageOffset, srcSize,
        VKW::RESOURCE_ACCESS_NONE, VKW::STAGE_BOTTOM,
        VKW::RESOURCE_ACCESS_TRANSFER_DST, VKW::STAGE_TRANSFER);

    context.CmdCopyBufferToBuffer(resource, storageOffset, src, srcOffset, srcSize);
    
    context.CmdResourceDependency(resource, storageOffset, GetSize(),
        VKW::RESOURCE_ACCESS_TRANSFER_DST, VKW::STAGE_TRANSFER,
        VKW::RESOURCE_ACCESS_NONE, VKW::STAGE_TOP);
}




}
