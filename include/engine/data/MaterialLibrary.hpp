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


    MaterialLibrary(DRE::DefaultAllocator* allocator);

    void InitDefaultMaterials();

    Material* CreateMaterial(Hash hash, char const* name);
    Material* CreateMaterial(std::uint32_t id, char const* sceneName, char const* name);
    Material* GetMaterial(Hash hash);
    Material* GetMaterial(std::uint32_t id, char const* sceneName);

    //void debug_output()
    //{
    //    m_MaterialsMap.ForEach([](auto const& pair) { std::cout << "K:" << *pair.key << " V:" << pair.value->GetName() << std::endl; });
    //}

private:
    DRE::HashTable<Hash, Material, DRE::DefaultAllocator> m_MaterialsMap;
};

}

