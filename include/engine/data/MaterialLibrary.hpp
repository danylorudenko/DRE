#pragma once

#include <class_features\NonCopyable.hpp>
#include <memory\Memory.hpp>

#include <foundation\Container\HashTable.hpp>
#include <foundation\String\InplaceString.hpp>

#include <data\Material.hpp>

namespace Data
{

///////////////////////////////////////
class MaterialLibrary
    : public NonCopyable
{
public:
    MaterialLibrary();

    void InitDefaultMaterials();

    Material* CreateMaterial(char const* name);


    Material* GetMaterial(char const* name);

private:
    DRE::HashTable<DRE::String64, Material, DRE::MainAllocator> m_MaterialsMap;
};

}

