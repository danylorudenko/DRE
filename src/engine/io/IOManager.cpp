#include <engine\io\IOManager.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <utility>
#include <charconv>
#include <filesystem>

#include <foundation\Common.hpp>
#include <foundation\memory\Memory.hpp>
#include <foundation\memory\ByteBuffer.hpp>
#include <foundation\Container\HashTable.hpp>
#include <foundation\system\Time.hpp>
#include <foundation\util\Hash.hpp>

#include <assimp\Importer.hpp>
#include <assimp\postprocess.h>
#include <assimp\scene.h>

#include <vk_wrapper\pipeline\ShaderModule.hpp>

#include <gfx\GraphicsManager.hpp>

#include <engine\data\GeometryLibrary.hpp>
#include <engine\data\MaterialLibrary.hpp>
#include <engine\io\ShaderDB.hpp>
#include <engine\scene\Scene.hpp>

#include <spirv_cross.hpp>

namespace IO
{

IOManager::IOManager(Data::MaterialLibrary* materialLibrary, Data::GeometryLibrary* geometryLibrary)
    : m_MaterialLibrary{ materialLibrary }
    , m_GeometryLibrary{ geometryLibrary }
{
}

IOManager::~IOManager() 
{
}

std::uint64_t IOManager::ReadFileToBuffer(char const* path, DRE::ByteBuffer* buffer)
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

    if (buffer == nullptr)
        return fileSize;

    buffer->Resize(fileSize);

    istream.seekg(0, std::ios_base::beg);
    istream.read(buffer->As<char*>(), static_cast<std::uint64_t>(fileSize));
    istream.close();

    return fileSize;
}

std::uint64_t IOManager::ReadFileStringToBuffer(char const* path, DRE::ByteBuffer* buffer)
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

    if (buffer == nullptr)
        return fileSize;

    buffer->Resize(static_cast<std::uint64_t>(fileSize) + 1);

    istream.seekg(0, std::ios_base::beg);
    istream.read(buffer->As<char*>(), static_cast<std::uint64_t>(fileSize));
    istream.close();

    buffer->As<char*>()[static_cast<std::uint64_t>(fileSize)] = '\0';

    return fileSize;
}

void IOManager::WriteNewFile(char const* path, DRE::ByteBuffer const& buffer)
{
    std::ofstream ostream{ path, std::ios_base::binary };
    if (!ostream) {
        std::cerr << "Error writing file in path: " << path << std::endl;
    }

    ostream.write(buffer.As<char const*>(), buffer.Size());

    if (!ostream) {
        std::cerr << "Failed to write " << buffer.Size() << " bytes to file " << path << std::endl;
    }

    ostream.close();
}

Data::Texture2D IOManager::ReadTexture2D(char const* path, Data::TextureChannelVariations channelVariations)
{
    Data::Texture2D texture;
    texture.ReadFromFile(path, channelVariations);
    return texture;
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
    case Data::Material::TextureProperty::ROUGHNESS:
        aiType = aiTextureType_DIFFUSE_ROUGHNESS;
        break;
    default:
        DRE_ASSERT(false, "Unsupported Data::Material::TextureProperty::Slot while parsing material textures.");
    }


    if (aiMat->GetTexture(aiType, 0, &aiTexturePath) != aiReturn_SUCCESS)
    {
        std::cout << "Warning: failed to find material texture flor slot " << slot << std::endl;
        return;
    }

    aiTexture const* tex = scene->GetEmbeddedTexture(aiTexturePath.C_Str());
    if (tex == nullptr)
    {
        DRE::String256 textureFilePath = assetFolderPath;
        char const separator[2] = { std::filesystem::path::preferred_separator, '\0' };
        textureFilePath.Append(separator);
        textureFilePath.Append(aiTexturePath.C_Str(), DRE::U16(aiTexturePath.length));

        Data::Texture2D dataTexture = ReadTexture2D(textureFilePath.GetData(), channels);
        GFX::Texture* gfxTexture = GFX::g_GraphicsManager->GetTextureBank().LoadTexture2DSync(
            dataTexture.GetName(),
            dataTexture.GetSizeX(),
            dataTexture.GetSizeY(),
            dataTexture.GetFormat(),
            dataTexture.GetBuffer()
        );

        material->AssignTextureToSlot(slot, DRE_MOVE(dataTexture), gfxTexture);
    }
    else
    {
        // process aiTexture
        // fuck this for now
        DRE_ASSERT(false, "assimp embedded textures are not yet supported");
    }
}

void IOManager::ParseAssimpNodeRecursive(VKW::Context& gfxContext,
    char const* assetPath,
    aiScene const* scene,
    char const* sceneName,
    aiNode const* node,
    WORLD::Scene& targetScene,
    WORLD::SceneNode* parentNode,
    ASGeometryVector& asGeometryVector,
    ASGeometryIndexCounts& asGeometryIndexCounts)
{
    aiMatrix4x4 const t = node->mTransformation;
    glm::mat4 const transform {
            t.a1, t.a2, t.a3, t.a4,
            t.b1, t.b2, t.b3, t.b4,
            t.c1, t.c2, t.c3, t.c4,
            t.d1, t.d2, t.d3, t.d4
    };

    WORLD::SceneNode* aggregatorNode = targetScene.CreateSceneNode(nullptr, parentNode);
    aggregatorNode->SetMatrix(transform);

    for (std::uint32_t i = 0, count = node->mNumMeshes; i < count; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

        Data::Material* material = m_MaterialLibrary->GetMaterial(mesh->mMaterialIndex, sceneName);
        Data::Geometry* geometry = m_GeometryLibrary->GetGeometry(node->mMeshes[i], sceneName);

        WORLD::Entity* entity = targetScene.CreateOpaqueEntity(gfxContext, geometry, material, aggregatorNode);
    }

    for (std::uint32_t i = 0; i < node->mNumChildren; i++)
    {
        ParseAssimpNodeRecursive(gfxContext, assetPath, scene, sceneName, node->mChildren[i], targetScene, aggregatorNode, asGeometryVector, asGeometryIndexCounts);
    }
}

WORLD::SceneNode* IOManager::ParseModelFile(char const* path, WORLD::Scene& targetScene, glm::mat4 baseTransform, Data::TextureChannelVariations metalnessRoughnessOverride)
{
    Assimp::Importer importer = Assimp::Importer();

    aiScene const* scene = importer.ReadFile(path, aiProcessPreset_TargetRealtime_Fast | aiProcess_FlipUVs);
    char const* sceneName = aiScene::GetShortFilename(path);

    DRE_ASSERT(scene != nullptr, "Failed to load a model file.");
    if (scene == nullptr)
        return nullptr;

    ParseAssimpMeshes(GFX::g_GraphicsManager->GetMainContext(), scene, sceneName);
    ParseAssimpMaterials(scene, sceneName, path, metalnessRoughnessOverride);

    WORLD::SceneNode* parentNode = targetScene.CreateSceneNode(nullptr, targetScene.GetRootNode());
    parentNode->SetMatrix(baseTransform);
    parentNode->SetName(sceneName);

    ASGeometryVector asGeometryVector{ &DRE::g_FrameScratchAllocator };
    ASGeometryIndexCounts asGeometryIndexCounts{ &DRE::g_FrameScratchAllocator };
    ParseAssimpNodeRecursive(GFX::g_GraphicsManager->GetMainContext(), path, scene, sceneName, scene->mRootNode, targetScene, parentNode, asGeometryVector, asGeometryIndexCounts);
    GFX::g_GraphicsManager->GetMainContext().FlushAll();

    return parentNode;
}

void IOManager::ParseAssimpMaterials(aiScene const* scene, char const* sceneName, char const* path, Data::TextureChannelVariations metalnessRoughnessOverride)
{
    // get folder with texture files
    DRE::String256 textureFilePath = path;
    std::uint8_t folderEnd = textureFilePath.GetSize() - 1;
    while (textureFilePath[folderEnd] != '\\' && textureFilePath[folderEnd] != '/')
    {
        DRE_ASSERT(folderEnd != 0, "Unable to find file path separator!");
        --folderEnd;
    }
    textureFilePath.Shrink(folderEnd);

    for (std::uint32_t i = 0, size = scene->mNumMaterials; i < size; i++)
    {
        aiMaterial* aiMat = scene->mMaterials[i];
        Data::Material* material = m_MaterialLibrary->CreateMaterial(i, sceneName, aiMat->GetName().C_Str());

        DRE_ASSERT(aiMat->GetTextureCount(aiTextureType_DIFFUSE) <= 1, "We don't support multiple textures of the same type per material (DIFFUSE).");
        DRE_ASSERT(aiMat->GetTextureCount(aiTextureType_NORMALS) <= 1, "We don't support multiple textures of the same type per material (NORMALS)");
        DRE_ASSERT(aiMat->GetTextureCount(aiTextureType_METALNESS) <= 1, "We don't support multiple textures of the same type per material (METALNESS)");
        DRE_ASSERT(aiMat->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) <= 1, "We don't support multiple textures of the same type per material (DIFFUSE_ROUGHNESS)");
        DRE_ASSERT(aiMat->GetTextureCount(aiTextureType_AMBIENT_OCCLUSION) <= 1, "We don't support multiple textures of the same type per material (AMBIENT_OCCLUSION)");

        // PROCESS TEXTURES
        ParseMaterialTexture(scene, aiMat, textureFilePath, material, Data::Material::TextureProperty::DIFFUSE, Data::TEXTURE_VARIATION_RGBA);
        ParseMaterialTexture(scene, aiMat, textureFilePath, material, Data::Material::TextureProperty::NORMAL, Data::TEXTURE_VARIATION_RGBA);

        if (metalnessRoughnessOverride == Data::TEXTURE_VARIATION_INVALID)
        {
            ParseMaterialTexture(scene, aiMat, textureFilePath, material, Data::Material::TextureProperty::METALNESS, Data::TEXTURE_VARIATION_GRAY);
            ParseMaterialTexture(scene, aiMat, textureFilePath, material, Data::Material::TextureProperty::ROUGHNESS, Data::TEXTURE_VARIATION_GRAY);
        }
        else
        {
            // it means we have texture with merged metalness and roughness attributes. Let it lie in metalness
            ParseMaterialTexture(scene, aiMat, textureFilePath, material, Data::Material::TextureProperty::METALNESS, metalnessRoughnessOverride);
        }
        // TODO: load rgb here

        material->GetRenderingProperties().SetMaterialType(Data::Material::RenderingProperties::MATERIAL_TYPE_OPAQUE);

        DRE::String64 materialName;
        materialName.Append(sceneName);
        materialName.Append("_");
        materialName.Append(material->GetName());

        GFX::Material* gfxMaterial = GFX::g_GraphicsManager->GetPipelineDB().CreateMaterial(materialName, material->GetRenderingProperties().GetMaterialType());
        material->FlushToGfxMaterial(gfxMaterial);
    }
}

void IOManager::ParseAssimpMeshes(VKW::Context& gfxContext, aiScene const* scene, char const* sceneName)
{
    for (std::uint32_t i = 0, size = scene->mNumMeshes; i < size; i++)
    {
        aiMesh* mesh = scene->mMeshes[i];

        Data::Geometry geometry{ sizeof(Data::DREVertex), sizeof(Data::DREIndex) };
        geometry.ResizeVertexStorage(mesh->mNumVertices);
        geometry.ResizeIndexStorage(mesh->mNumFaces * 3);

        for (std::uint32_t j = 0, jSize = mesh->mNumVertices; j < jSize; j++)
        {
            Data::DREVertex& v = geometry.GetVertex<Data::DREVertex>(j);
            v.pos[0] = mesh->mVertices[j].x;
            v.pos[1] = mesh->mVertices[j].y;
            v.pos[2] = mesh->mVertices[j].z;

            v.norm[0] = mesh->mNormals[j].x;
            v.norm[1] = mesh->mNormals[j].y;
            v.norm[2] = mesh->mNormals[j].z;

            v.tan[0] = mesh->mTangents[j].x;
            v.tan[1] = mesh->mTangents[j].y;
            v.tan[2] = mesh->mTangents[j].z;

            v.btan[0] = mesh->mBitangents[j].x;
            v.btan[1] = mesh->mBitangents[j].y;
            v.btan[2] = mesh->mBitangents[j].z;

            v.uv0[0] = mesh->mTextureCoords[0][j].x;
            v.uv0[1] = mesh->mTextureCoords[0][j].y;
        }

        for (std::uint32_t j = 0, jSize = mesh->mNumFaces; j < jSize; j++)
        {
            geometry.GetIndex<Data::DREIndex>(j*3 + 0) = mesh->mFaces[j].mIndices[0];
            geometry.GetIndex<Data::DREIndex>(j*3 + 1) = mesh->mFaces[j].mIndices[1];
            geometry.GetIndex<Data::DREIndex>(j*3 + 2) = mesh->mFaces[j].mIndices[2];
        }

        m_GeometryLibrary->AddGeometry(i, sceneName, DRE_MOVE(geometry));
        GFX::g_GraphicsManager->GetRayTracignManager().RegisterGeometry(m_GeometryLibrary->GetGeometry(i, sceneName), gfxContext);
    }
}

//
//void IOManager::LoadShaderBinaries()
//{
//    std::filesystem::recursive_directory_iterator dir_iterator{ "shaders", std::filesystem::directory_options::follow_directory_symlink };
//
//    for (auto const& entry : dir_iterator)
//    {
//        if (entry.path().has_extension() && entry.path().extension() == ".spv")
//        {
//            DRE::ByteBuffer moduleBuffer{ static_cast<std::uint64_t>(entry.file_size()) };
//            ReadFileToBuffer(entry.path().generic_string().c_str(), &moduleBuffer);
//
//            
//        }
//    }
//
//    m_ShaderObserverThread = std::thread{ &IOManager::ShaderObserver, this };
//}

}
