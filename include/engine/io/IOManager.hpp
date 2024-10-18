#pragma once

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>

#include <foundation\memory\Memory.hpp>
#include <foundation\container\HashTable.hpp>

#include <vk_wrapper\pipeline\ShaderModule.hpp>

#include <engine\data\Texture2D.hpp>
#include <engine\data\Material.hpp>
#include <engine\data\Geometry.hpp>

#include <glm\mat4x4.hpp>
#include <glm\gtc\matrix_transform.hpp>

#include <assimp\matrix4x4.h>

#include <thread>
#include <atomic>
#include <mutex>

#define DEBUG_SHADER_COMPILATION

namespace WORLD
{
class Scene;
class SceneNode;
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

namespace WORLD
{
class SceneNode;
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

        DRE::InplaceVector<Member, 16> m_Members;
        std::uint8_t m_PushConstantSize     : 7;
        std::uint8_t m_PushConstantPresent  : 1;
        VKW::DescriptorStage m_PushConstantStages = VKW::DESCRIPTOR_STAGE_NONE;

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

    WORLD::SceneNode* ParseModelFile(char const* path, WORLD::Scene& targetScene, char const* defaultShader, glm::mat4 baseTransform = glm::identity<glm::mat4>(), Data::TextureChannelVariations metalnessRoughnessOverride = Data::TEXTURE_VARIATION_INVALID);


    static std::uint64_t    ReadFileToBuffer(char const* path, DRE::ByteBuffer* buffer);
    static std::uint64_t    ReadFileStringToBuffer(char const* path, DRE::ByteBuffer* buffer);
    static void             WriteNewFile(char const* path, DRE::ByteBuffer const& buffer);

    DRE::ByteBuffer         CompileGLSL(char const* file);
    std::mutex&             GetShaderIncluderMutex() { return m_ShaderIncluderMutex; }

    inline bool                             NewShadersPending() { return IOManager::m_PendingChangesFlag.load(std::memory_order::acquire); }
    DRE::InplaceVector<DRE::String64, 12>   GetPendingShaders();

private:
    void ParseAssimpMeshes(VKW::Context& gfxContext, aiScene const* scene, char const* sceneName);
    void ParseAssimpMaterials(aiScene const* scene, char const* sceneName, char const* path, char const* defaultShader, Data::TextureChannelVariations metalnessRoughnessOverride);
    void ParseAssimpNodeRecursive(VKW::Context& gfxContext, char const* assetPath, aiScene const* scene, char const* sceneName, aiNode const* node, WORLD::Scene& targetScene, WORLD::SceneNode* parentNode);

    void BuildAssimpNodeAccelerationStructure(VKW::Context& gfxContext, char const* assetPath, aiScene const* scene, char const* sceneName, aiNode const* node, WORLD::Scene& targetScene, WORLD::SceneNode* parentNode, Data::Material* mat, Data::Geometry* geometry);

    void ParseMaterialTexture(aiScene const* scene, aiMaterial const* aiMat, DRE::String256 const& assetFolderPath, Data::Material* material, Data::Material::TextureProperty::Slot slot, Data::TextureChannelVariations channels);

    void ShaderObserver();

private:
    DRE::DefaultAllocator* m_Allocator;

    Data::MaterialLibrary* m_MaterialLibrary;
    Data::GeometryLibrary* m_GeometryLibrary;

    DRE::HashTable<DRE::String64, ShaderData, DRE::DefaultAllocator> m_ShaderData;

    std::mutex  m_ShaderIncluderMutex;

    DRE::InplaceVector<DRE::String64, 12> m_PendingShaders;
    std::mutex  m_PendingShadersMutex;
    std::thread m_ShaderObserverThread;
    std::atomic_bool m_PendingChangesFlag;

#ifdef DEBUG_SHADER_COMPILATION
    std::mutex  m_DebugShaderCompilationMutex;
#endif

};

}

