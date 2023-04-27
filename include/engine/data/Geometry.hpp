#pragma once

#include <glm\vec3.hpp>
#include <glm\vec2.hpp>

#include <foundation\memory\Memory.hpp>
#include <foundation\memory\ByteBuffer.hpp>
#include <foundation\memory\Pointer.hpp>

namespace Data
{

class Geometry
{
public:
    Geometry(std::uint16_t vertexStride, std::uint16_t indexSize);

    Geometry(Geometry&& rhs);
    Geometry& operator=(Geometry&& rhs);

    inline void SetVertexData(DRE::ByteBuffer&& data) { m_VertexStorage = DRE_MOVE(data); }
    inline void SetIndexData(DRE::ByteBuffer&& data) { m_IndexStorage = DRE_MOVE(data); }

    inline void ResizeVertexStorage(std::uint32_t count)
    {
        m_VertexStorage.Resize(count * m_VertexStride);
    }

    inline void ResizeIndexStorage(std::uint32_t count)
    {
        m_IndexStorage.Resize(count * m_IndexSize);
    }

    template<typename T>
    T& GetVertex(std::uint32_t i) { return *reinterpret_cast<T*>(DRE::PtrAdd(m_VertexStorage.Data(), i * m_VertexStride)); }

    template<typename T>
    T& GetIndex(std::uint32_t i) { return *reinterpret_cast<T*>(DRE::PtrAdd(m_IndexStorage.Data(), i * m_IndexSize)); }
    
    inline std::uint32_t GetVertexSizeInBytes() const { return m_VertexStorage.Size(); }
    inline std::uint32_t GetIndexSizeInBytes() const { return m_IndexStorage.Size(); }

    inline std::uint32_t GetVertexCount() const { return m_VertexStorage.Size() / m_VertexStride; }
    inline std::uint32_t GetIndexCount() const { return m_IndexStorage.Size() / m_IndexSize; }

    inline void const*   GetVertexData() const { return m_VertexStorage.Data(); }
    inline void const*   GetIndexData() const { return m_IndexStorage.Data(); }

    inline bool          HasIndexBuffer() const { return m_IndexStorage.Size() != 0; }

private:
    std::uint16_t m_VertexStride;
    std::uint16_t m_IndexSize;

    DRE::ByteBuffer m_VertexStorage;
    DRE::ByteBuffer m_IndexStorage;
};

}

