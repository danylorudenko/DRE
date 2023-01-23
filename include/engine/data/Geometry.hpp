#pragma once

#include <glm\vec3.hpp>
#include <glm\vec2.hpp>

#include <foundation\memory\Memory.hpp>
#include <foundation\container\Vector.hpp>

namespace Data
{

class Geometry
{
public:
    Geometry(DRE::DefaultAllocator* allocator);

    Geometry(Geometry&& rhs);
    Geometry& operator=(Geometry&& rhs);


    using Index = std::uint32_t;

    struct Vertex
    {
        float pos[3];
        float norm[3];
        float tan[3];
        float btan[3];
        float uv0[2];
    };

    inline void SetVertexCount(std::uint32_t count)
    {
        m_VertexStorage.Resize(count);
    }

    inline void SetIndexCount(std::uint32_t count)
    {
        m_IndexStorage.Resize(count);
    }

    inline Vertex const& GetVertex(std::uint32_t i) const { return m_VertexStorage[i]; }
    inline Index const& GetIndex(std::uint32_t i) const { return m_IndexStorage[i]; }

    inline Vertex& GetVertex(std::uint32_t i) { return m_VertexStorage[i]; }
    inline Index& GetIndex(std::uint32_t i) { return m_IndexStorage[i]; }

    inline std::uint32_t GetVertexSizeInBytes() const { return m_VertexStorage.SizeInBytes(); }
    inline std::uint32_t GetIndexSizeInBytes() const { return m_IndexStorage.SizeInBytes(); }

    inline std::uint32_t GetVertexCount() const { return m_VertexStorage.Size(); }
    inline std::uint32_t GetIndexCount() const { return m_IndexStorage.Size(); }

    inline Vertex const* GetVertexData() const { return m_VertexStorage.Data(); }
    inline Index const*  GetIndexData() const { return m_IndexStorage.Data(); }

    inline bool          HasIndexBuffer() const { return !m_IndexStorage.Empty(); }

private:
    DRE::Vector<Vertex, DRE::DefaultAllocator>  m_VertexStorage;
    DRE::Vector<Index, DRE::DefaultAllocator>   m_IndexStorage;
};

}

