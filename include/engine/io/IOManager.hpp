#pragma once

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>

#include <foundation\memory\Memory.hpp>
#include <foundation\container\HashTable.hpp>

#include <vk_wrapper\pipeline\ShaderModule.hpp>

#include <engine\data\Texture2D.hpp>
#include <engine\data\Material.hpp>
#include <engine\data\Geometry.hpp>
#include <engine\io\ShaderDB.hpp>

#include <glm\mat4x4.hpp>
#include <glm\gtc\matrix_transform.hpp>

#include <assimp\matrix4x4.h>

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

namespace GFX
{
class PipelineDB;
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
    IOManager(Data::MaterialLibrary* materialLibrary, Data::GeometryLibrary* geometryLibrary);
    ~IOManager();

    Data::Texture2D ReadTexture2D(char const* path, Data::TextureChannelVariations channels);

    WORLD::SceneNode* ParseModelFile(char const* path, WORLD::Scene& targetScene, glm::mat4 baseTransform = glm::identity<glm::mat4>(), Data::TextureChannelVariations metalnessRoughnessOverride = Data::TEXTURE_VARIATION_INVALID);


    static std::uint64_t    ReadFileToBuffer(char const* path, DRE::ByteBuffer* buffer);
    static std::uint64_t    ReadFileStringToBuffer(char const* path, DRE::ByteBuffer* buffer);
    static void             WriteNewFile(char const* path, DRE::ByteBuffer const& buffer);

private:
    void ParseAssimpMeshes(VKW::Context& gfxContext, aiScene const* scene, char const* sceneName);
    void ParseAssimpMaterials(aiScene const* scene, char const* sceneName, char const* path, Data::TextureChannelVariations metalnessRoughnessOverride);

    using ASGeometryIndexCounts = DRE::Vector<std::uint32_t, DRE::AllocatorLinear>;
    using ASGeometryVector = DRE::Vector<VkAccelerationStructureGeometryKHR, DRE::AllocatorLinear>;

    void ParseAssimpNodeRecursive(
        VKW::Context& gfxContext,
        char const* assetPath,
        aiScene const* scene,
        char const* sceneName,
        aiNode const* node,
        WORLD::Scene& targetScene,
        WORLD::SceneNode* parentNode,
        ASGeometryVector& asGeometryVector,
        ASGeometryIndexCounts& asGeometryIndexCounts);

    void ParseMaterialTexture(aiScene const* scene, aiMaterial const* aiMat, DRE::String256 const& assetFolderPath, Data::Material* material, Data::Material::TextureProperty::Slot slot, Data::TextureChannelVariations channels);

private:
    Data::MaterialLibrary*  m_MaterialLibrary;
    Data::GeometryLibrary*  m_GeometryLibrary;


};

}

