#pragma once

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>
#include <engine\data\Model.hpp>
#include <engine\data\ModelMesh.hpp>
#include <engine\data\Texture2D.hpp>
#include <engine\data\MaterialLibrary.hpp>

#include <spirv_cross.hpp>

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

        DRE::InplaceVector<Member, 12> m_Members;

        void Merge(ShaderInterface const& rhs);
    };

    struct ShaderData
    {
        DRE::ByteBuffer m_Binary;
        spv::ExecutionModel m_ExecutionModel;
        ShaderInterface m_Interface;
    };

public:
    IOManager(Data::MaterialLibrary* materialLibrary);

    void LoadShaderFiles();
    ShaderData* GetShaderData(char const* name) { return m_ShaderBinaries.Find(name).value; }

    Data::ModelMesh ReadModelMesh(char const* path);
    Data::Texture2D ReadTexture2D(char const* path, Data::TextureChannelVariations channels);

    void            ParseModelFile(char const* path, WORLD::Scene& targetScene);

    ~IOManager();

    static std::uint64_t ReadFileToBuffer(char const* path, DRE::ByteBuffer& buffer);

private:
    void ParseAssimpNodeRecursive(VKW::Context& gfxContext, char const* assetPath, aiScene const* scene, aiNode const* node, WORLD::Scene& targetScene);

    Data::Material* ParseMeshMaterial(aiMesh const* mesh, char const* assetPath, aiScene const* scene, Data::MaterialLibrary* materialLibrary);
    void ParseMaterialTexture(aiScene const* scene, aiMaterial const* aiMat, DRE::String256 const& assetFolderPath, Data::Material* material, Data::Material::TextureProperty::Slot slot, Data::TextureChannelVariations channels);

private:
    Data::MaterialLibrary* m_MaterialLibrary;

    DRE::HashTable<DRE::String64, ShaderData, DRE::MainAllocator> m_ShaderBinaries;
};

}

