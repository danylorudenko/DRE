#pragma once

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>

#include <foundation\memory\Memory.hpp>
#include <foundation\container\HashTable.hpp>

#include <vk_wrapper\pipeline\ShaderModule.hpp>

#include <engine\data\Texture2D.hpp>
#include <engine\data\Material.hpp>

#include <assimp\matrix4x4.h>

#include <thread>
#include <atomic>
#include <mutex>

namespace WORLD
{
class Scene;
class Entity;
}

namespace VKW
{
class Context;
}

namespace Data
{
class MaterialLibrary;
class GeometryLibrary;
}

struct aiScene;
struct aiNode;
struct aiMesh;
struct aiMaterial;

namespace IO
{

class IOManager
    : public NonCopyable
    , public NonMovable
{
public:
    struct ShaderInterface
    {
        struct Member 
        {
            VKW::DescriptorType type;
            VKW::DescriptorStage stage;
            std::uint8_t set;
            std::uint8_t binding;
            std::uint8_t arraySize;

            bool operator==(Member const& rhs) const;
            bool operator!=(Member const& rhs) const;
        };

        DRE::InplaceVector<Member, 9> m_Members;
        std::uint8_t m_PushBufferBinding : 7;
        std::uint8_t m_PushBufferPresent : 1;

        void Merge(ShaderInterface const& rhs);
    };

    struct ShaderData
    {
        DRE::ByteBuffer m_Binary;
        VKW::ShaderModuleType m_ModuleType;
        ShaderInterface m_Interface;
    };

public:
    IOManager(DRE::DefaultAllocator* allocator, Data::MaterialLibrary* materialLibrary, Data::GeometryLibrary* geometryLibrary);
    ~IOManager();

    void LoadShaderBinaries();
    void CompileGLSLSources();
    ShaderData* GetShaderData(char const* name) { return m_ShaderData.Find(name).value; }

    Data::Texture2D ReadTexture2D(char const* path, Data::TextureChannelVariations channels);
    void            ParseModelFile(char const* path, WORLD::Scene& targetScene);


    static std::uint64_t    ReadFileToBuffer(char const* path, DRE::ByteBuffer* buffer);
    static std::uint64_t    ReadFileStringToBuffer(char const* path, DRE::ByteBuffer* buffer);
    static void             WriteNewFile(char const* path, DRE::ByteBuffer const& buffer);
    static DRE::ByteBuffer  CompileGLSL(char const* file);

    inline bool                             NewShadersPending() { return IOManager::m_PendingChangesFlag.load(std::memory_order::acquire); }
    DRE::InplaceVector<DRE::String64, 12>   GetPendingShaders();

private:
    void ParseAssimpMeshes(VKW::Context& gfxContext, aiScene const* scene);
    void ParseAssimpMaterials(aiScene const* scene, char const* path);
    void ParseAssimpNodeRecursive(VKW::Context& gfxContext, char const* assetPath, aiScene const* scene, aiNode const* node, aiMatrix4x4 const& parentTransform, WORLD::Scene& targetScene);

    void ParseMaterialTexture(aiScene const* scene, aiMaterial const* aiMat, DRE::String256 const& assetFolderPath, Data::Material* material, Data::Material::TextureProperty::Slot slot, Data::TextureChannelVariations channels);

    void ShaderObserver();

private:
    DRE::DefaultAllocator* m_Allocator;

    Data::MaterialLibrary* m_MaterialLibrary;
    Data::GeometryLibrary* m_GeometryLibrary;

    DRE::HashTable<DRE::String64, ShaderData, DRE::DefaultAllocator> m_ShaderData;

    DRE::InplaceVector<DRE::String64, 12> m_PendingShaders;
    std::mutex  m_PendingShadersMutex;
    std::thread m_ShaderObserverThread;
    std::atomic_bool m_PendingChangesFlag;


};

}

