#pragma once

#include <foundation\class_features\NonCopyable.hpp>

#include <foundation\Common.hpp>
#include <foundation\memory\Memory.hpp>
#include <foundation\container\HashTable.hpp>

#include <engine\data\Geometry.hpp>

namespace Data
{

class GeometryLibrary
    : public NonCopyable
{
public:
    struct Hash
    {
        explicit Hash(DRE::U32 id, char const* sceneName);
        explicit Hash(char const* name);
        explicit Hash(DRE::U32 hash) : m_Hash{ hash } {}
        explicit Hash() : m_Hash{ 0 } {}
        explicit operator DRE::U32() const { return m_Hash; }

        bool operator==(Hash const& rhs) { return m_Hash == rhs.m_Hash; }
        bool operator!=(Hash const& rhs) { return m_Hash != rhs.m_Hash; }

    private:
        DRE::U32 m_Hash;
    };

    GeometryLibrary(DRE::DefaultAllocator* allocator);

    inline Geometry* GetGeometry(Hash hash) { return m_Geometries.Find(hash).value; }
    inline Geometry* GetGeometry(std::uint32_t id, char const* sceneName) { return m_Geometries.Find(Hash(id, sceneName)).value; }
    inline Geometry* GetGeometry(char const* name) { return m_Geometries.Find(Hash(name)).value; }

    void AddGeometry(DRE::U32 id, char const* sceneName, Geometry&& data);
    void AddGeometry(Hash hash, Geometry&& data);
    void AddGeometry(char const* name, Geometry&& data);

    void LoadDefaultGeometry();

private:
    static void GeneratePlaneMesh(DRE::U32 width, DRE::U32 height, DRE::ByteBuffer& vertexOut, DRE::ByteBuffer& indexOut);
    static void GenerateSphereMesh(DRE::U32 verticalResolution, DRE::U32 horizontalResolution, DRE::ByteBuffer& vertexOut, DRE::ByteBuffer& indexOut);

private:
    DRE::HashTable<Hash, Geometry, DRE::DefaultAllocator> m_Geometries;
};

}

