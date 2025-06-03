#pragma once

#include <glm\vec3.hpp>
#include <glm\vec2.hpp>

#include <foundation\memory\Memory.hpp>
#include <foundation\memory\ByteBuffer.hpp>
#include <foundation\memory\Pointer.hpp>

namespace Data
{

struct DREVertex
{
    float pos[3];
    float norm[3];
    float tan[3];
    float btan[3];
    float uv0[2];
};

using DREIndex = DRE::U32;

class Geometry
{
public:
    Geometry(DRE::U16 vertexStride, DRE::U16 indexSize);

    Geometry(Geometry&& rhs);
    Geometry& operator=(Geometry&& rhs);

    inline void SetVertexData(DRE::ByteBuffer&& data) { m_VertexStorage = DRE_MOVE(data); }
    inline void SetIndexData(DRE::ByteBuffer&& data) { m_IndexStorage = DRE_MOVE(data); }

    inline void ResizeVertexStorage(DRE::U32 count)
    {
        m_VertexStorage.Resize(count * m_VertexStride);
    }

    inline void ResizeIndexStorage(DRE::U32 count)
    {
        m_IndexStorage.Resize(count * m_IndexSize);
    }

    template<typename T>
    T& GetVertex(DRE::U32 i) { return *reinterpret_cast<T*>(DRE::PtrAdd(m_VertexStorage.Data(), i * m_VertexStride)); }

    template<typename T>
    T& GetIndex(DRE::U32 i) { return *reinterpret_cast<T*>(DRE::PtrAdd(m_IndexStorage.Data(), i * m_IndexSize)); }
    
    inline DRE::U32 GetVertexSizeInBytes() const { return m_VertexStorage.Size(); }
    inline DRE::U32 GetIndexSizeInBytes() const { return m_IndexStorage.Size(); }

    inline DRE::U32 GetVertexCount() const { return m_VertexStorage.Size() / DRE::Max(m_VertexStride, DRE::U16{ 1 }); }
    inline DRE::U32 GetIndexCount() const { return m_IndexStorage.Size() / DRE::Max(m_IndexSize, DRE::U16{ 1 }); }

    inline void const*   GetVertexData() const { return m_VertexStorage.Data(); }
    inline void const*   GetIndexData() const { return m_IndexStorage.Data(); }

    inline bool          HasIndexBuffer() const { return m_IndexStorage.Size() != 0; }

private:
    DRE::U16 m_VertexStride;
    DRE::U16 m_IndexSize;

    DRE::ByteBuffer m_VertexStorage;
    DRE::ByteBuffer m_IndexStorage;
};

}

