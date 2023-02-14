#include <foundation\memory\ByteBuffer.hpp>

DRE_BEGIN_NAMESPACE

ByteBuffer::ByteBuffer()
    : buffer_{ nullptr }
    , size_{ 0 }
    , capacity_{ 0 }
{}

ByteBuffer::ByteBuffer(std::uint64_t size)
    : buffer_{ nullptr }
    , size_{ 0 }
    , capacity_{ 0 }
{
    Resize(size);
}

ByteBuffer::ByteBuffer(void* srcData, std::uint64_t size)
    : buffer_{ nullptr }
    , size_{ 0 }
    , capacity_{ 0 }
{
    Resize(size);
    std::memcpy(buffer_, srcData, size);
}

ByteBuffer::ByteBuffer(ByteBuffer const& rhs)
    : buffer_{ nullptr }
    , size_{ 0 }
    , capacity_{ 0 }
{
    operator=(rhs);
}

ByteBuffer::ByteBuffer(ByteBuffer&& rhs)
    : buffer_{ nullptr }
    , size_{ 0 }
    , capacity_{ 0 }
{
    operator=(std::move(rhs));
}

ByteBuffer& ByteBuffer::operator=(ByteBuffer const& rhs)
{
    auto size = rhs.Size();
    Resize(size);
    std::memcpy(buffer_, rhs.Data(), size);

    return *this;
}

ByteBuffer& ByteBuffer::operator=(ByteBuffer&& rhs)
{
    DRE_SWAP_MEMBER(buffer_);
    DRE_SWAP_MEMBER(size_);
    DRE_SWAP_MEMBER(capacity_);

    return *this;
}

std::uint64_t ByteBuffer::Size() const
{
    return size_;
}

void* ByteBuffer::Data() const
{
    return buffer_;
}

void ByteBuffer::Resize(std::uint64_t newSize)
{
    if (newSize <= size_)
    {
        size_ = newSize;
        return;
    }

    void* newBuffer = malloc(newSize);

    if (buffer_) {
        std::memcpy(newBuffer, buffer_, newSize);
        free(buffer_);
    }

    buffer_ = newBuffer;
    size_ = newSize;
    capacity_ = newSize;
}

ByteBuffer::~ByteBuffer()
{
    std::free(buffer_);
}

DRE_END_NAMESPACE
