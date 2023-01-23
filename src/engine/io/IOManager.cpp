#include <engine\io\IOManager.hpp>

#include <fstream>
#include <iostream>
#include <utility>
#include <charconv>
#include <filesystem>

#include <foundation\memory\Memory.hpp>
#include <foundation\memory\ByteBuffer.hpp>
#include <foundation\Container\HashTable.hpp>
#include <foundation\util\Hash.hpp>

#include <assimp\Importer.hpp>
#include <assimp\postprocess.h>
#include <assimp\scene.h>

#include <vk_wrapper\pipeline\ShaderModule.hpp>

#include <gfx\GraphicsManager.hpp>

#include <engine\data\GeometryLibrary.hpp>
#include <engine\data\MaterialLibrary.hpp>
#include <engine\scene\Scene.hpp>

#include <spirv_cross\spirv_cross.hpp>

namespace IO
{

IOManager::IOManager(DRE::DefaultAllocator* allocator, Data::MaterialLibrary* materialLibrary, Data::GeometryLibrary* geometryLibrary)
    : m_Allocator{ allocator }
    , m_MaterialLibrary{ materialLibrary }
    , m_GeometryLibrary{ geometryLibrary }
    , m_ShaderBinaries{ allocator }
{
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

void IOManager::ParseAssimpNodeRecursive(VKW::Context& gfxContext, char const* assetPath, aiScene const* scene, aiNode const* node, WORLD::Scene& targetScene)
{
    for (std::uint32_t i = 0, count = node->mNumMeshes; i < count; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

        Data::Material* material = m_MaterialLibrary->GetMaterial(mesh->mMaterialIndex);
        Data::Geometry* geometry = m_GeometryLibrary->GetGeometry(node->mMeshes[i]);
        WORLD::Entity& nodeEntity = targetScene.CreateRenderableEntity(gfxContext, geometry, material);
    }

    for (std::uint32_t i = 0; i < node->mNumChildren; i++)
    {
        ParseAssimpNodeRecursive(gfxContext, assetPath, scene, node->mChildren[i], targetScene);
    }
}

void IOManager::ParseModelFile(char const* path, WORLD::Scene& targetScene)
{
    Assimp::Importer importer = Assimp::Importer();

    aiScene const* scene = importer.ReadFile(path, aiProcessPreset_TargetRealtime_Fast);

    DRE_ASSERT(scene != nullptr, "Failed to load a model file.");
    if (scene == nullptr)
        return;

    ParseAssimpMeshes(GFX::g_GraphicsManager->GetMainContext(), scene);
    ParseAssimpMaterials(scene, path);
    ParseAssimpNodeRecursive(GFX::g_GraphicsManager->GetMainContext(), path, scene, scene->mRootNode, targetScene);
    GFX::g_GraphicsManager->GetMainContext().FlushAll();
}

void IOManager::ParseAssimpMaterials(aiScene const* scene, char const* path)
{
    // get folder with texture files
    DRE::String256 textureFilePath = path;
    std::uint8_t folderEnd = textureFilePath.GetSize() - 1;
    while (textureFilePath[folderEnd] != '\\' && textureFilePath[folderEnd] != '/')
    {
        DRE_ASSERT(folderEnd != 0, "Unable to find file path separator!");
        --folderEnd;
    }
    textureFilePath.Shrink(folderEnd + 1);

    for (std::uint32_t i = 0, size = scene->mNumMaterials; i < size; i++)
    {
        aiMaterial* aiMat = scene->mMaterials[i];
        Data::Material* material = m_MaterialLibrary->CreateMaterial(i, aiMat->GetName().C_Str());

        DRE_ASSERT(aiMat->GetTextureCount(aiTextureType_DIFFUSE) <= 1, "We don't support multiple textures of the same type per material (DIFFUSE).");
        DRE_ASSERT(aiMat->GetTextureCount(aiTextureType_NORMALS) <= 1, "We don't support multiple textures of the same type per material (NORMALS)");

        // PROCESS TEXTURES
        ParseMaterialTexture(scene, aiMat, textureFilePath, material, Data::Material::TextureProperty::DIFFUSE, Data::TEXTURE_VARIATION_RGB);
        ParseMaterialTexture(scene, aiMat, textureFilePath, material, Data::Material::TextureProperty::NORMAL, Data::TEXTURE_VARIATION_RGB);

        material->GetRenderingProperties().SetMaterialType(Data::Material::RenderingProperties::MATERIAL_TYPE_DEFAULT_LIT);
    }
}

void IOManager::ParseAssimpMeshes(VKW::Context& gfxContext, aiScene const* scene)
{
    for (std::uint32_t i = 0, size = scene->mNumMeshes; i < size; i++)
    {
        aiMesh* mesh = scene->mMeshes[i];

        Data::Geometry geometry{ m_Allocator };
        geometry.SetVertexCount(mesh->mNumVertices);
        geometry.SetIndexCount(mesh->mNumFaces * 3);

        for (std::uint32_t j = 0, jSize = mesh->mNumVertices; j < jSize; j++)
        {
            Data::Geometry::Vertex& v = geometry.GetVertex(j);
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
            geometry.GetIndex(j*3 + 0) = mesh->mFaces[j].mIndices[0];
            geometry.GetIndex(j*3 + 1) = mesh->mFaces[j].mIndices[1];
            geometry.GetIndex(j*3 + 2) = mesh->mFaces[j].mIndices[2];
        }

        m_GeometryLibrary->AddGeometry(i, DRE_MOVE(geometry));
    }
}

void IOManager::ShaderInterface::Merge(IOManager::ShaderInterface const& rhs)
{
    for (std::uint32_t i = 0, size = rhs.m_Members.Size(); i < size; i++)
    {
        if (m_Members.Find(rhs.m_Members[i]) != m_Members.Size())
            continue;

        m_Members.EmplaceBack(rhs.m_Members[i]);
    }
}

bool IOManager::ShaderInterface::Member::operator==(IOManager::ShaderInterface::Member const& rhs) const
{
    return 
        (type == rhs.type) &&
        //(stage == rhs.stage) && this will differ, because we're merging different stages
        (set == rhs.set) &&
        (binding == rhs.binding) &&
        (arraySize == rhs.arraySize);
}

bool IOManager::ShaderInterface::Member::operator!=(IOManager::ShaderInterface::Member const& rhs) const
{
    return !operator==(rhs);
}

VKW::ShaderModuleType SPVExecutionModelToVKWType(spv::ExecutionModel executionModel)
{
    switch (executionModel)
    {
    case spv::ExecutionModelVertex:
        return VKW::SHADER_MODULE_TYPE_VERTEX;
    case spv::ExecutionModelFragment:
        return VKW::SHADER_MODULE_TYPE_FRAGMENT;
    case spv::ExecutionModelGLCompute:
        return VKW::SHADER_MODULE_TYPE_COMPUTE;
    default:
        return VKW::SHADER_MODULE_TYPE_NONE;
    }
}

template<typename TResourceArray>
void ParseShaderInterfaceType(spirv_cross::Compiler& compiler, TResourceArray const& resources, VKW::DescriptorType type, IOManager::ShaderInterface& resultInterface)
{
    for (spirv_cross::Resource const& res : resources)
    {
        auto& member        = resultInterface.m_Members.EmplaceBack();
        member.type         = type;
        member.stage        = SPVExecutionModelToVKWType(compiler.get_execution_model());
        member.set          = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
        member.binding      = compiler.get_decoration(res.id, spv::DecorationBinding);

        spirv_cross::SPIRType const& spvType = compiler.get_type(res.type_id);
        member.arraySize    = spvType.array.empty() ? 1 : spvType.array[0];

        DRE_ASSERT(spvType.array.size() <= 1, "Don't support multidimentional array relfection yet.");
    }
}

void ParseShaderInterface(spirv_cross::Compiler& compiler, IOManager::ShaderInterface& resultInterface)
{
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    ParseShaderInterfaceType(compiler, resources.storage_images,    VKW::DESCRIPTOR_TYPE_STORAGE_IMAGE, resultInterface);
    ParseShaderInterfaceType(compiler, resources.separate_images,   VKW::DESCRIPTOR_TYPE_TEXTURE, resultInterface);
    ParseShaderInterfaceType(compiler, resources.separate_samplers, VKW::DESCRIPTOR_TYPE_SAMPLER, resultInterface);
    ParseShaderInterfaceType(compiler, resources.uniform_buffers,   VKW::DESCRIPTOR_TYPE_UNIFORM_BUFFER, resultInterface);

    DRE_ASSERT(resources.push_constant_buffers.size() <= 1, "Don't support multiple push buffers in reflection yet.");

    if (!resources.push_constant_buffers.empty())
    {
        spirv_cross::Resource const& res = resources.push_constant_buffers[0];
        resultInterface.m_PushBufferBinding = compiler.get_decoration(res.id, spv::DecorationBinding);
        resultInterface.m_PushBufferPresent = 1;
    }
}

void IOManager::LoadShaderFiles()
{
    std::filesystem::recursive_directory_iterator dir_iterator(".", std::filesystem::directory_options::none);
    for (auto const& entry : dir_iterator)
    {
        if (entry.path().has_extension() && entry.path().extension() == ".spv")
        {
            // .stem() is a filename without extension
            ShaderData& shaderData = m_ShaderBinaries.Emplace(entry.path().stem().generic_string().c_str());

            DRE::ByteBuffer moduleBuffer{ static_cast<std::uint64_t>(entry.file_size()) };
            ReadFileToBuffer(entry.path().generic_string().c_str(), moduleBuffer);
            shaderData.m_Binary = DRE_MOVE(moduleBuffer);
            
            spirv_cross::Compiler compiler{ reinterpret_cast<std::uint32_t const*>(shaderData.m_Binary.Data()), shaderData.m_Binary.Size() / sizeof(std::uint32_t) };
            shaderData.m_ModuleType = SPVExecutionModelToVKWType(compiler.get_execution_model());

            ParseShaderInterface(compiler, shaderData.m_Interface); 
        }
    }
}


}
