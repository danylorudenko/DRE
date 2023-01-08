#include <engine\io\IOManager.hpp>

#include <fstream>
#include <iostream>
#include <utility>
#include <charconv>
#include <filesystem>

#include <foundation\memory\Memory.hpp>
#include <foundation\memory\ByteBuffer.hpp>

#include <assimp\Importer.hpp>
#include <assimp\postprocess.h>
#include <assimp\scene.h>

#include <foundation\Container\HashTable.hpp>

#include <engine\data\MaterialLibrary.hpp>

#include <gfx\GraphicsManager.hpp>

#include <engine\scene\Scene.hpp>

#include <spirv_cross\spirv_cross.hpp>

namespace IO
{

IOManager::IOManager(Data::MaterialLibrary* materialLibrary)
    : m_MaterialLibrary{ materialLibrary }
{
    LoadShaders();
}

IOManager::~IOManager() {}

std::uint64_t IOManager::ReadFileToBuffer(char const* path, DRE::ByteBuffer& buffer)
{
    std::ifstream istream{ path, std::ios_base::binary | std::ios_base::beg };
    if (!istream) {
        std::cerr << "Error opening file in path: " << path << std::endl;
        return 0;
    }

    auto const fileSize = istream.seekg(0, std::ios_base::end).tellg();
    if (!istream) {
        std::cerr << "Error measuring file size: " << path << std::endl;
        return 0;
    }

    buffer.Resize(fileSize);

    istream.seekg(0, std::ios_base::beg);
    istream.read(buffer.As<char*>(), static_cast<std::uint64_t>(fileSize));
    istream.close();

    return fileSize;
}

struct ModelHeader
{
    std::uint32_t vertexCount_ = 0;
    std::uint32_t vertexSize_ = 0;
    std::uint32_t indexCount_ = 0;
    std::uint32_t indexSize_ = 0;
    std::uint32_t vertexContentFlags_ = 0;
};

Data::ModelMesh IOManager::ReadModelMesh(char const* path)
{
    std::ifstream istream{ path, std::ios_base::binary | std::ios_base::beg };
    if (!istream) {
        std::cerr << "Error opening ModelMesh in path: " << path << std::endl;
        return Data::ModelMesh{};
    }

    auto const fileSize = istream.seekg(0, std::ios_base::end).tellg();
    if (!istream) {
        std::cerr << "Error measuring file size: " << path << std::endl;
        return Data::ModelMesh();
    }


    ModelHeader modelHeader{};

    istream.seekg(0, std::ios_base::beg);
    istream.read(reinterpret_cast<char*>(&modelHeader), sizeof(modelHeader));

    Data::ModelMesh model;

    std::uint32_t const vertexDataSizeBytes = modelHeader.vertexCount_ * modelHeader.vertexSize_;
    std::uint32_t const indexDataSizeBytes = modelHeader.indexCount_ * modelHeader.indexSize_;

    model.vertexData_.resize(vertexDataSizeBytes);
    model.indexData_.resize(indexDataSizeBytes);
    model.vertexContentFlags_ = modelHeader.vertexContentFlags_;

    istream.read(reinterpret_cast<char*>(model.vertexData_.data()), vertexDataSizeBytes);
    istream.read(reinterpret_cast<char*>(model.indexData_.data()), indexDataSizeBytes);

    istream.close();

    return model;
}

Data::Texture2D IOManager::ReadTexture2D(char const* path, Data::TextureChannelVariations channelVariations)
{
    Data::Texture2D texture;
    texture.ReadFromFile(path, channelVariations);
    return texture;
}

template<typename T>
void WriteMemorySequence(void*& memory, T& data)
{
    std::memcpy(memory, &data, sizeof(data));
    memory = DRE::PtrAdd(memory, sizeof(data));
}

void IOManager::ParseMaterialTexture(aiScene const* scene, aiMaterial const* aiMat, DRE::String256 const& assetFolderPath, Data::Material* material, Data::Material::TextureProperty::Slot slot, Data::TextureChannelVariations channels)
{
    aiString aiTexturePath;
    aiTextureType aiType = aiTextureType_NONE;
    switch (slot)
    {
    case Data::Material::TextureProperty::DIFFUSE:
        aiType = aiTextureType_DIFFUSE;
        break;
    case Data::Material::TextureProperty::NORMAL:
        aiType = aiTextureType_NORMALS;
        break;
    case Data::Material::TextureProperty::METALNESS:
        aiType = aiTextureType_METALNESS;
        break;
    default:
        DRE_ASSERT(false, "Unsupported Data::Material::TextureProperty::Slot while parsing material textures.");
    }


    DRE_ASSERT(aiMat->GetTexture(aiType, 0, &aiTexturePath) == 0, "Failed to get diffuse texture");
    aiTexture const* tex = scene->GetEmbeddedTexture(aiTexturePath.C_Str());
    if (tex == nullptr)
    {
        DRE::String256 textureFilePath = assetFolderPath;
        textureFilePath.Append(aiTexturePath.C_Str(), DRE::U16(aiTexturePath.length));

        Data::Texture2D dataTexture = ReadTexture2D(textureFilePath, channels);
        material->AssignTextureToSlot(slot, DRE_MOVE(dataTexture));
    }
    else
    {
        // process aiTexture
        // fuck this for now
        DRE_ASSERT(false, "assimp embedded textures are not yet supported");
    }
}

Data::Material* IOManager::ParseMeshMaterial(aiMesh const* mesh, char const* assetPath, aiScene const* scene, Data::MaterialLibrary* materialLibrary)
{
    /*auto result = meshMaterialMap.Find(mesh);
    if (result.value != nullptr)
        return ;*/

    //parse material
    aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
    aiString aiName = mat->GetName();

    char name[32];
    if (aiName.length == 0)
    {
        std::to_chars_result r = std::to_chars(name, name + 31, mesh->mMaterialIndex);
        DRE_ASSERT(r.ec == std::errc{}, "Failed to convert int to string (std::to_chars).");
        *r.ptr = '\0';
    }
    else
    {
        std::strcpy(name, aiName.C_Str());
    }

    Data::Material* material = m_MaterialLibrary->GetMaterial(name);
    if (material != nullptr)
        return material;

    material = m_MaterialLibrary->CreateMaterial(name);

    DRE_ASSERT(mat->GetTextureCount(aiTextureType_DIFFUSE) <= 1, "We don't support multiple textures of the same type per material (DIFFUSE).");
    DRE_ASSERT(mat->GetTextureCount(aiTextureType_NORMALS) <= 1, "We don't support multiple textures of the same type per material (NORMALS)");
    //DRE_ASSERT(mat->GetTextureCount(aiTextureType_METALNESS) <= 1, "We don't support multiple textures of the same type per material (METALNESS)");

    // get folder with texture files
    DRE::String256 textureFilePath = assetPath;
    std::uint16_t folderEnd = textureFilePath.GetSize() - 1;
    while (textureFilePath[folderEnd] != '\\' && textureFilePath[folderEnd] != '/')
    {
        DRE_ASSERT(folderEnd != 0, "Unable to find file path separator!");
        --folderEnd;
    }
    textureFilePath.Shrink(folderEnd + 1);
    
    // PROCESS TEXTURES
    ParseMaterialTexture(scene, mat, textureFilePath, material, Data::Material::TextureProperty::DIFFUSE, Data::TEXTURE_VARIATION_RGB);
    ParseMaterialTexture(scene, mat, textureFilePath, material, Data::Material::TextureProperty::NORMAL, Data::TEXTURE_VARIATION_RGB);
    //ParseMaterialTexture(scene, mat, textureFilePath, material, Data::Material::TextureProperty::METALNESS, Data::TEXTURE_VARIATION_GRAY);

    return material;
}

void WriteMemorySequence(void*& memory, void* data, std::uint32_t size)
{
    std::memcpy(memory, &data, size);
    memory = DRE::PtrAdd(memory, size);
}

void IOManager::ParseAssimpNodeRecursive(VKW::Context& gfxContext, char const* assetPath, aiScene const* scene, aiNode const* node, WORLD::Scene& targetScene)
{
    for (std::uint32_t i = 0, count = node->mNumMeshes; i < count; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        std::uint32_t const vertexMemoryRequirements 
            = sizeof(aiVector3D) * mesh->mNumVertices * 4   // position + tangent space
            + sizeof(aiVector2D) * mesh->mNumVertices;      // UV

        std::uint32_t const indexMemoryRequirements = (sizeof(aiVector3D) * mesh->mNumFaces);

        std::uint32_t const meshMemoryRequirements = vertexMemoryRequirements + indexMemoryRequirements;


        auto meshMemory = GFX::g_GraphicsManager->GetUploadArena().AllocateTransientRegion(GFX::g_GraphicsManager->GetCurrentFrameID(), meshMemoryRequirements, 256);
        void* memorySequence = meshMemory.m_MappedRange;

        for (std::uint32_t v = 0, vCount = mesh->mNumVertices; v < vCount; v++)
        {
            WriteMemorySequence(memorySequence, mesh->mVertices[v]);
            WriteMemorySequence(memorySequence, mesh->mNormals[v]);
            WriteMemorySequence(memorySequence, mesh->mTangents[v]);
            WriteMemorySequence(memorySequence, mesh->mBitangents[v]);
            WriteMemorySequence(memorySequence, mesh->mTextureCoords[0][v]);
        }

        VKW::BufferResource* vertexBuffer = GFX::g_GraphicsManager->GetMainDevice()->GetResourcesController()->CreateBuffer(vertexMemoryRequirements, VKW::BufferUsage::VERTEX_INDEX);
        VKW::BufferResource* indexBuffer = GFX::g_GraphicsManager->GetMainDevice()->GetResourcesController()->CreateBuffer(indexMemoryRequirements, VKW::BufferUsage::VERTEX_INDEX);

        void* indexStart = memorySequence;

        for (std::uint32_t f = 0, fCount = mesh->mNumFaces; f < fCount; f++)
        {
            WriteMemorySequence(memorySequence, mesh->mFaces[f].mIndices, sizeof(unsigned int));
        }

        meshMemory.FlushCaches();

        gfxContext.CmdResourceDependency(meshMemory.m_Buffer, meshMemory.m_OffsetInBuffer, meshMemory.m_Size, VKW::RESOURCE_ACCESS_HOST_WRITE, VKW::STAGE_HOST, VKW::RESOURCE_ACCESS_TRANSFER_SRC, VKW::STAGE_TRANSFER);

        // schedule copy
        gfxContext.CmdResourceDependency(vertexBuffer, VKW::RESOURCE_ACCESS_NONE, VKW::STAGE_TOP, VKW::RESOURCE_ACCESS_TRANSFER_DST, VKW::STAGE_TRANSFER);
        gfxContext.CmdCopyBufferToBuffer(vertexBuffer, 0, meshMemory.m_Buffer, meshMemory.m_OffsetInBuffer, vertexMemoryRequirements);

        gfxContext.CmdResourceDependency(indexBuffer, VKW::RESOURCE_ACCESS_NONE, VKW::STAGE_TOP, VKW::RESOURCE_ACCESS_TRANSFER_DST, VKW::STAGE_TRANSFER);

        Data::Material* material = ParseMeshMaterial(mesh, assetPath, scene, m_MaterialLibrary);
        WORLD::Entity& nodeEntity = targetScene.CreateEntity(material);
    }

    for (std::uint32_t i = 0; i < node->mNumChildren; i++)
    {
        ParseAssimpNodeRecursive(gfxContext, assetPath, scene, node->mChildren[i], targetScene);
    }
}

void IOManager::ParseModelFile(char const* path, WORLD::Scene& targetScene)
{
    Assimp::Importer importer = Assimp::Importer();

    aiScene const* scene = importer.ReadFile(path, aiProcess_CalcTangentSpace | aiProcess_Triangulate);

    DRE_ASSERT(scene != nullptr, "Failed to load a model file.");
    if (scene == nullptr)
        return;

    ParseAssimpNodeRecursive(GFX::g_GraphicsManager->GetMainContext(), path, scene, scene->mRootNode, targetScene);
}

void IOManager::LoadShaders()
{
    std::filesystem::path currentDir = std::filesystem::current_path();
    std::cout << currentDir.generic_u8string();
    std::cout << std::endl;

    std::filesystem::recursive_directory_iterator dir_iterator(currentDir, std::filesystem::directory_options::none);
    for (auto const& dir : dir_iterator)
    {
        if (dir.path().has_extension() && dir.path().extension() == ".spv")
        {
            
        }

    }
}


}
