#pragma once

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\memory\Memory.hpp>

#include <foundation\Container\HashTable.hpp>

#include <engine\data\Material.hpp>

#include <iostream>

namespace Data
{

///////////////////////////////////////
class MaterialLibrary
    : public NonCopyable
{
public:
    static constexpr char NAME_DEFAULT_DIFFUSE_WHITE[] = "dre_default_diffuse";

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


    MaterialLibrary(DRE::DefaultAllocator* allocator);

    void InitDefaultMaterials();

    Material* CreateMaterial(char const* name);
    Material* CreateMaterial(Hash hash, char const* name);
    Material* CreateMaterial(DRE::U32 id, char const* sceneName, char const* name);

    Material* GetMaterial(char const* name);
    Material* GetMaterial(Hash hash);
    Material* GetMaterial(DRE::U32 id, char const* sceneName);

private:
    DRE::HashTable<Hash, Material, DRE::DefaultAllocator> m_MaterialsMap;
    DRE::U32 m_MaterialIDCounter;
};

}

