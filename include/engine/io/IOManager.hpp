#pragma once

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>
#include <engine\data\Model.hpp>
#include <engine\data\ModelMesh.hpp>
#include <engine\data\Texture2D.hpp>
#include <engine\data\MaterialLibrary.hpp>

namespace WORLD
{
class Scene;
class Entity;
}

namespace VKW
{
class Context;
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
    IOManager(Data::MaterialLibrary* materialLibrary);

    static std::uint64_t ReadFileToBuffer(char const* path, DRE::ByteBuffer& buffer);

    Data::ModelMesh ReadModelMesh(char const* path);
    Data::Texture2D ReadTexture2D(char const* path, Data::TextureChannelVariations channels);

    void            ParseModelFile(char const* path, WORLD::Scene& targetScene);

    ~IOManager();

private:
    //using MaterialMap = DRE::HashTable<aiMesh const*, aiMaterial const*, DRE::MainAllocator>;
    void ParseAssimpNodeRecursive(VKW::Context& gfxContext, char const* assetPath, aiScene const* scene, aiNode const* node, WORLD::Scene& targetScene/*, MaterialMap& meshMaterialMap*/);

    Data::Material* ParseMeshMaterial(aiMesh const* mesh, char const* assetPath, aiScene const* scene, Data::MaterialLibrary* materialLibrary/*, MaterialMap& meshMaterialMap*/);
    void ParseMaterialTexture(aiScene const* scene, aiMaterial const* aiMat, DRE::String256 const& assetFolderPath, Data::Material* material, Data::Material::TextureProperty::Slot slot, Data::TextureChannelVariations channels);

private:
    Data::MaterialLibrary* m_MaterialLibrary;
};

}

