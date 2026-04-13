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
#include <foundation\system\Parallel.hpp>

#include <assimp\Importer.hpp>
#include <assimp\postprocess.h>
#include <assimp\scene.h>
#include <assimp\GltfMaterial.h>

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

void IOManager::ParseMaterialTexture_Parallel(aiScene const* scene, aiMaterial const* aiMat, DRE::String256 const& assetFolderPath, Data::Material* material, Data::Material::TextureProperty::Slot slot, Data::TextureChannelVariations channels)
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
    case Data::Material::TextureProperty::OPACITY:
        aiType = aiTextureType_OPACITY;
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
        GFX::Texture* gfxTexture = nullptr;

        {
            std::lock_guard<std::mutex> guard{ m_TextureLoadingMutex };
            gfxTexture = GFX::g_GraphicsManager->GetTextureBank().LoadTexture2DSync(
                dataTexture.GetName(),
                dataTexture.GetSizeX(),
                dataTexture.GetSizeY(),
                dataTexture.GetFormat(),
                dataTexture.GetBuffer()
            );
        }

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
        t.a1, t.b1, t.c1, t.d1,
        t.a2, t.b2, t.c2, t.d2,
        t.a3, t.b3, t.c3, t.d3,
        t.a4, t.b4, t.c4, t.d4
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

WORLD::SceneNode* IOManager::ParseModelFile(char const* path, WORLD::Scene& targetScene, glm::mat4 baseTransform)
{
    Assimp::Importer importer = Assimp::Importer();

    aiScene const* scene = importer.ReadFile(path,
        aiProcess_CalcTangentSpace |
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_Triangulate |
        aiProcess_GenUVCoords |
        aiProcess_SortByPType |
        aiProcess_FlipUVs
    );
    char const* sceneName = aiScene::GetShortFilename(path);

    DRE_ASSERT(scene != nullptr, "Failed to load a model file.");
    if (scene == nullptr)
        return nullptr;

    ParseAssimpMeshes(GFX::g_GraphicsManager->GetMainContext(), scene, sceneName);
    ParseAssimpMaterials(scene, sceneName, path);

    WORLD::SceneNode* parentNode = targetScene.CreateSceneNode(nullptr, targetScene.GetRootNode());
    parentNode->SetMatrix(baseTransform);
    parentNode->SetName(sceneName);

    ASGeometryVector asGeometryVector{ &DRE::g_FrameScratchAllocator };
    ASGeometryIndexCounts asGeometryIndexCounts{ &DRE::g_FrameScratchAllocator };
    ParseAssimpNodeRecursive(GFX::g_GraphicsManager->GetMainContext(), path, scene, sceneName, scene->mRootNode, targetScene, parentNode, asGeometryVector, asGeometryIndexCounts);
    GFX::g_GraphicsManager->GetMainContext().FlushAll();

    return parentNode;
}

Data::Material::RenderingProperties::MaterialType MaterialTypeFromAssimpMaterial(aiMaterial const* aiMaterial)
{
    aiString alphaMode;
    if (aiMaterial->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == aiReturn_SUCCESS)
    {
        if (alphaMode == aiString{ "MASK" })
            return Data::Material::RenderingProperties::MATERIAL_TYPE_ALPHA_MASKED;
        else if (alphaMode == aiString{ "BLEND" })
            return Data::Material::RenderingProperties::MATERIAL_TYPE_ALPHA_MASKED; // we don't support blended materials for now, so we will treat them as masked
    }

    aiBlendMode blendMode = aiBlendMode_Default;
    aiMaterial->Get(AI_MATKEY_BLEND_FUNC, blendMode);
    if (blendMode == aiBlendMode_Additive)
        return Data::Material::RenderingProperties::MATERIAL_TYPE_ALPHA_MASKED;

    float opacity = 1.0f;
    aiMaterial->Get(AI_MATKEY_OPACITY, opacity);
    if (opacity < 1.0f)
        return Data::Material::RenderingProperties::MATERIAL_TYPE_ALPHA_MASKED;

    DRE::U32 opacityTextureCount = aiMaterial->GetTextureCount(aiTextureType_OPACITY);
    if (opacityTextureCount > 0)
        return Data::Material::RenderingProperties::MATERIAL_TYPE_ALPHA_MASKED;

    return Data::Material::RenderingProperties::MATERIAL_TYPE_OPAQUE;
}

bool IsMaterialRoughnessDiffuseCombined(aiMaterial const* aiMaterial)
{
    aiString metallicRoughnessTexturePath;
    if (aiMaterial->GetTexture(aiTextureType_GLTF_METALLIC_ROUGHNESS, 0, &metallicRoughnessTexturePath) == aiReturn_SUCCESS)
        return true;

    // let's have it here just in case, very weird check. Maybe other formats (not GLTF) also do this
    aiString metallicPath, roughnessPath;

    bool const hasMetalness = aiMaterial->GetTexture(aiTextureType_METALNESS, 0, &metallicPath) == aiReturn_SUCCESS;
    bool const hasRoughness = aiMaterial->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &roughnessPath) == aiReturn_SUCCESS;

    if (hasMetalness && hasRoughness)
    {
        if (std::strcmp(metallicPath.C_Str(), roughnessPath.C_Str()) == 0)
        {
            DRE_ASSERT(false, "This is a very weird check, and we need to be extra cautious when it's triggered.");
            return true;
        }
    }

    return false;
}

bool IsMaterialNormalTextureSet(aiMaterial const* aiMaterial)
{
    return aiMaterial->GetTextureCount(aiTextureType_NORMALS) > 0;
}

void IOManager::ParseAssimpMaterials(aiScene const* scene, char const* sceneName, char const* path)
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

    std::mutex materialMutex;

    DRE::ParallelFor<16>(scene->mNumMaterials, 
    [this, scene, sceneName, &textureFilePath, &materialMutex](DRE::U32 index)
    {
        aiMaterial const* aiMat = scene->mMaterials[index];

        Data::Material* material = nullptr;
        {
            std::lock_guard<std::mutex> guard{ materialMutex };
            material = m_MaterialLibrary->CreateMaterial(index, sceneName, aiMat->GetName().C_Str());
        }

        material->GetRenderingProperties().SetMaterialType(MaterialTypeFromAssimpMaterial(aiMat));

        DRE_ASSERT(aiMat->GetTextureCount(aiTextureType_DIFFUSE) <= 1, "We don't support multiple textures of the same type per material (DIFFUSE).");
        DRE_ASSERT(aiMat->GetTextureCount(aiTextureType_NORMALS) <= 1, "We don't support multiple textures of the same type per material (NORMALS)");
        DRE_ASSERT(aiMat->GetTextureCount(aiTextureType_METALNESS) <= 1, "We don't support multiple textures of the same type per material (METALNESS)");
        DRE_ASSERT(aiMat->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) <= 1, "We don't support multiple textures of the same type per material (DIFFUSE_ROUGHNESS)");
        DRE_ASSERT(aiMat->GetTextureCount(aiTextureType_AMBIENT_OCCLUSION) <= 1, "We don't support multiple textures of the same type per material (AMBIENT_OCCLUSION)");

        // PROCESS TEXTURES
        ParseMaterialTexture_Parallel(scene, aiMat, textureFilePath, material, Data::Material::TextureProperty::DIFFUSE, Data::TEXTURE_VARIATION_RGBA);
        ParseMaterialTexture_Parallel(scene, aiMat, textureFilePath, material, Data::Material::TextureProperty::NORMAL, Data::TEXTURE_VARIATION_RGBA);

        if (IsMaterialRoughnessDiffuseCombined(aiMat))
        {
            // it means we have texture with merged metalness and roughness attributes. Let it lie in metalness
            ParseMaterialTexture_Parallel(scene, aiMat, textureFilePath, material, Data::Material::TextureProperty::METALNESS, Data::TEXTURE_VARIATION_RGBA);
            material->GetRenderingProperties().EnableMaterialTexturesMetallicRoughnessCombined(true);
        }
        else
        {
            ParseMaterialTexture_Parallel(scene, aiMat, textureFilePath, material, Data::Material::TextureProperty::METALNESS, Data::TEXTURE_VARIATION_GRAY);
            ParseMaterialTexture_Parallel(scene, aiMat, textureFilePath, material, Data::Material::TextureProperty::ROUGHNESS, Data::TEXTURE_VARIATION_GRAY);
            material->GetRenderingProperties().EnableMaterialTexturesDefault(true);
        }

        if (IsMaterialNormalTextureSet(aiMat))
        {
            material->GetRenderingProperties().EnableNormalTexture(true);
        }
        else
        {
            material->GetRenderingProperties().EnableNormalTBN(true);
        }

        ParseMaterialTexture_Parallel(scene, aiMat, textureFilePath, material, Data::Material::TextureProperty::OPACITY, Data::TEXTURE_VARIATION_GRAY);

        {
            std::lock_guard<std::mutex> guard{ materialMutex };
            GFX::Material* gfxMaterial = GFX::g_GraphicsManager->GetPipelineDB().CreateMaterial(material->GetName(), material->GetRenderingProperties().GetMaterialType());
            material->FlushToGfxMaterial(gfxMaterial);
        }
    }
    , true);
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

}
