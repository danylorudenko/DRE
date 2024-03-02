#pragma once

#include <foundation\class_features\NonCopyable.hpp>

#include <foundation\memory\Memory.hpp>
#include <foundation\container\HashTable.hpp>
#include <foundation\string\InplaceString.hpp>

#include <engine\data\Geometry.hpp>

namespace Data
{

class GeometryLibrary
    : public NonCopyable
{
public:
    struct Hash
    {
        explicit Hash(std::uint32_t id, char const* sceneName);
        explicit Hash(std::uint32_t hash) : m_Hash{ hash } {}
        explicit Hash() : m_Hash{ 0 } {}
        explicit operator std::uint32_t() const { return m_Hash; }

        bool operator==(Hash const& rhs) { return m_Hash == rhs.m_Hash; }
        bool operator!=(Hash const& rhs) { return m_Hash != rhs.m_Hash; }

    private:
        std::uint32_t m_Hash;
    };

    GeometryLibrary(DRE::DefaultAllocator* allocator);

    inline Geometry* GetGeometry(Hash hash) { return m_Geometries.Find(hash).value; }
    inline Geometry* GetGeometry(std::uint32_t id, char const* sceneName) { return m_Geometries.Find(Hash(id, sceneName)).value; }

    void AddGeometry(std::uint32_t id, char const* sceneName, Geometry&& data);
    void AddGeometry(Hash hash, Geometry&& data);


private:
    DRE::HashTable<Hash, Geometry, DRE::DefaultAllocator> m_Geometries;
};

}

