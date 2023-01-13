#include <engine\io\IOManager.hpp>

#include <fstream>
#include <iostream>
#include <utility>
#include <charconv>
#include <filesystem>

#include <foundation\memory\Memory.hpp>
#include <foundation\memory\ByteBuffer.hpp>
#include <foundation\Container\HashTable.hpp>

#include <assimp\Importer.hpp>
#include <assimp\postprocess.h>
#include <assimp\scene.h>

#include <vk_wrapper\pipeline\ShaderModule.hpp>

#include <gfx\GraphicsManager.hpp>

#include <engine\data\MaterialLibrary.hpp>
#include <engine\scene\Scene.hpp>

#include <spirv_cross\spirv_cross.hpp>

namespace IO
{

IOManager::IOManager(Data::MaterialLibrary* materialLibrary)
    : m_MaterialLibrary{ materialLibrary }
    , m_ShaderBinaries{ &DRE::g_MainAllocator }
{
    LoadShaderFiles();
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

Data::Material* IOManager::ParseMeshMaterial(aiMesh const* mesh, char const* assetPath, aiScene const* scene, Data::MaterialLibrary* materialLibrary)
{
    //parse material
    aiMaterial* aiMat = scene->mMaterials[mesh->mMaterialIndex];
    aiString aiName = aiMat->GetName();

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

    DRE_ASSERT(aiMat->GetTextureCount(aiTextureType_DIFFUSE) <= 1, "We don't support multiple textures of the same type per material (DIFFUSE).");
    DRE_ASSERT(aiMat->GetTextureCount(aiTextureType_NORMALS) <= 1, "We don't support multiple textures of the same type per material (NORMALS)");

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
    ParseMaterialTexture(scene, aiMat, textureFilePath, material, Data::Material::TextureProperty::DIFFUSE, Data::TEXTURE_VARIATION_RGB);
    ParseMaterialTexture(scene, aiMat, textureFilePath, material, Data::Material::TextureProperty::NORMAL, Data::TEXTURE_VARIATION_RGB);

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
            ShaderData& shaderData = m_ShaderBinaries.Emplace(entry.path().stem().u8string().c_str());

            DRE::ByteBuffer moduleBuffer{ static_cast<std::uint64_t>(entry.file_size()) };
            ReadFileToBuffer(entry.path().u8string().c_str(), moduleBuffer);
            shaderData.m_Binary = DRE_MOVE(moduleBuffer);
            
            spirv_cross::Compiler compiler{ reinterpret_cast<std::uint32_t const*>(shaderData.m_Binary.Data()), shaderData.m_Binary.Size() / sizeof(std::uint32_t) };
            shaderData.m_ModuleType = SPVExecutionModelToVKWType(compiler.get_execution_model());

            ParseShaderInterface(compiler, shaderData.m_Interface); 
        }
    }
}


}
