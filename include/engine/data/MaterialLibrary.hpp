#pragma once

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\memory\Memory.hpp>

#include <foundation\Container\HashTable.hpp>

#include <engine\data\Material.hpp>

namespace Data
{

///////////////////////////////////////
class MaterialLibrary
    : public NonCopyable
{
public:
    MaterialLibrary(DRE::DefaultAllocator* allocator);

    void InitDefaultMaterials();

    Material* CreateMaterial(std::uint32_t id, char const* name);
    Material* GetMaterial(std::uint32_t id);

private:
    DRE::HashTable<std::uint32_t, Material, DRE::DefaultAllocator> m_MaterialsMap;
};

}

