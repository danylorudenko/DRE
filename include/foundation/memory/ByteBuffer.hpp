#pragma once

#include <foundation\Common.hpp>

DRE_BEGIN_NAMESPACE

class ByteBuffer
{
public:
    ByteBuffer();
    ByteBuffer(std::uint64_t size);
    ByteBuffer(void* dataSrc, std::uint64_t size);
    ByteBuffer(ByteBuffer const& rhs);
    ByteBuffer(ByteBuffer&& rhs);

    ByteBuffer& operator=(ByteBuffer const& rhs);
    ByteBuffer& operator=(ByteBuffer&& rhs);

    std::uint64_t Size() const;

    void Resize(std::uint64_t newSize);
    void* Data() const;

    template<typename T>
    T As() const
    {
        return reinterpret_cast<T>(buffer_);
    }

    ~ByteBuffer();

private:
    void* buffer_;
    std::uint64_t size_;
    std::uint64_t capacity_;
};

DRE_END_NAMESPACE
