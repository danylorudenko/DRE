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
    GeometryLibrary(DRE::DefaultAllocator* allocator);

    inline Geometry* GetGeometry(std::uint32_t id) { return m_Geometries.Find(id).value; }
    void AddGeometry(std::uint32_t id, Geometry&& data);


private:
    DRE::HashTable<std::uint32_t, Geometry, DRE::DefaultAllocator> m_Geometries;
};

}

