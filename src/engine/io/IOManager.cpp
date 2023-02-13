#include <engine\io\IOManager.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <utility>
#include <charconv>
#include <filesystem>

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
#include <engine\scene\Scene.hpp>

#include <spirv_cross.hpp>

namespace IO
{

IOManager::IOManager(DRE::DefaultAllocator* allocator, Data::MaterialLibrary* materialLibrary, Data::GeometryLibrary* geometryLibrary)
    : m_Allocator{ allocator }
    , m_MaterialLibrary{ materialLibrary }
    , m_GeometryLibrary{ geometryLibrary }
    , m_ShaderData{ allocator }
    , m_PendingChangesFlag{ false }
{
}

IOManager::~IOManager() 
{
    // if we detach frie
    m_ShaderObserverThread.detach();
}

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

DRE::String64 IOManager::GetPendingShader()
{
    std::lock_guard guard(m_PendingShadersMutex);
    DRE_ASSERT(m_PendingShaders.Size() <= 1, "Multiple shader reload is not supported.");
    return m_PendingShaders.Empty() ? DRE::String64("") : m_PendingShaders[0];
}

void IOManager::SignalShadersProcessed()
{
    std::lock_guard guard(m_PendingShadersMutex);
    m_PendingShaders.Clear();
    IOManager::m_PendingChangesFlag.store(false, std::memory_order::relaxed);
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
    case Data::Material::TextureProperty::ROUGHNESS:
        aiType = aiTextureType_DIFFUSE_ROUGHNESS;
        break;
    case Data::Material::TextureProperty::OCCLUSION:
        aiType = aiTextureType_AMBIENT_OCCLUSION;
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

void IOManager::ParseAssimpNodeRecursive(VKW::Context& gfxContext, char const* assetPath, aiScene const* scene, aiNode const* node, aiMatrix4x4 const& parentTransform, WORLD::Scene& targetScene)
{
    aiMatrix4x4 const t = node->mTransformation * parentTransform;
    WORLD::Entity::TransformData const transform {{
            t.a1, t.a2, t.a3, t.a4,
            t.b1, t.b2, t.b3, t.b4,
            t.c1, t.c2, t.c3, t.c4,
            t.d1, t.d2, t.d3, t.d4
    }};

    for (std::uint32_t i = 0, count = node->mNumMeshes; i < count; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

        Data::Material* material = m_MaterialLibrary->GetMaterial(mesh->mMaterialIndex);
        Data::Geometry* geometry = m_GeometryLibrary->GetGeometry(node->mMeshes[i]);
        WORLD::Entity& nodeEntity = targetScene.CreateRenderableEntity(gfxContext, transform, geometry, material);
    }

    for (std::uint32_t i = 0; i < node->mNumChildren; i++)
    {
        ParseAssimpNodeRecursive(gfxContext, assetPath, scene, node->mChildren[i], t, targetScene);
    }
}

void IOManager::ParseModelFile(char const* path, WORLD::Scene& targetScene)
{
    Assimp::Importer importer = Assimp::Importer();

    aiScene const* scene = importer.ReadFile(path, aiProcessPreset_TargetRealtime_Fast | aiProcess_FlipUVs);

    DRE_ASSERT(scene != nullptr, "Failed to load a model file.");
    if (scene == nullptr)
        return;

    ParseAssimpMeshes(GFX::g_GraphicsManager->GetMainContext(), scene);
    ParseAssimpMaterials(scene, path);

    aiMatrix4x4 rootTransform{};
    ParseAssimpNodeRecursive(GFX::g_GraphicsManager->GetMainContext(), path, scene, scene->mRootNode, rootTransform, targetScene);
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
        DRE_ASSERT(aiMat->GetTextureCount(aiTextureType_METALNESS) <= 1, "We don't support multiple textures of the same type per material (METALNESS)");
        DRE_ASSERT(aiMat->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) <= 1, "We don't support multiple textures of the same type per material (DIFFUSE_ROUGHNESS)");
        DRE_ASSERT(aiMat->GetTextureCount(aiTextureType_AMBIENT_OCCLUSION) <= 1, "We don't support multiple textures of the same type per material (AMBIENT_OCCLUSION)");

        // PROCESS TEXTURES
        ParseMaterialTexture(scene, aiMat, textureFilePath, material, Data::Material::TextureProperty::DIFFUSE, Data::TEXTURE_VARIATION_RGBA);
        ParseMaterialTexture(scene, aiMat, textureFilePath, material, Data::Material::TextureProperty::NORMAL, Data::TEXTURE_VARIATION_RGBA);
        ParseMaterialTexture(scene, aiMat, textureFilePath, material, Data::Material::TextureProperty::METALNESS, Data::TEXTURE_VARIATION_GRAY);
        ParseMaterialTexture(scene, aiMat, textureFilePath, material, Data::Material::TextureProperty::ROUGHNESS, Data::TEXTURE_VARIATION_GRAY);
        ParseMaterialTexture(scene, aiMat, textureFilePath, material, Data::Material::TextureProperty::OCCLUSION, Data::TEXTURE_VARIATION_GRAY);
        // TODO: load rgb here

        material->GetRenderingProperties().SetMaterialType(Data::Material::RenderingProperties::MATERIAL_TYPE_COOK_TORRANCE);
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
        std::uint32_t result = m_Members.Find(rhs.m_Members[i]);
        if (result != m_Members.Size()) // similar member found
        {
            m_Members[result].stage = VKW::DescriptorStage(m_Members[result].stage | std::uint16_t(rhs.m_Members[i].stage));
            continue;
        }

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

VKW::ShaderModuleType SPVExecutionModelToVKWModuleType(spv::ExecutionModel executionModel)
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

VKW::DescriptorStage SPVExecutionModelToVKWStage(spv::ExecutionModel executionModel)
{
    switch (executionModel)
    {
    case spv::ExecutionModelVertex:
        return VKW::DESCRIPTOR_STAGE_VERTEX;
    case spv::ExecutionModelFragment:
        return VKW::DESCRIPTOR_STAGE_FRAGMENT;
    case spv::ExecutionModelGLCompute:
        return VKW::DESCRIPTOR_STAGE_COMPUTE;
    default:
        return VKW::DESCRIPTOR_STAGE_NONE;
    }
}

template<typename TResourceArray>
void ParseShaderInterfaceType(spirv_cross::Compiler& compiler, TResourceArray const& resources, VKW::DescriptorType type, IOManager::ShaderInterface& resultInterface)
{
    for (spirv_cross::Resource const& res : resources)
    {
        auto& member        = resultInterface.m_Members.EmplaceBack();
        member.type         = type;
        member.stage        = SPVExecutionModelToVKWStage(compiler.get_execution_model());
        member.set          = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
        member.binding      = compiler.get_decoration(res.id, spv::DecorationBinding);

        spirv_cross::SPIRType const& spvType = compiler.get_type(res.type_id);
        member.arraySize    = 1;

        if (!spvType.array.empty())
        {
            DRE_ASSERT(spvType.array.size() <= 1, "Don't support multidimentional array relfection yet.");
            DRE_ASSERT(spvType.array_size_literal[0] == true, "Arrays of size from specialization constants are not supported yet.");
            std::uint8_t const arraySize = spvType.array[0];
            member.arraySize = arraySize == 0 ? DRE_U8_MAX : arraySize; // if array size 0 -> variable size
        }
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
    std::filesystem::recursive_directory_iterator dir_iterator("shaders", std::filesystem::directory_options::follow_directory_symlink);
    for (auto const& entry : dir_iterator)
    {
        if (entry.path().has_extension() && entry.path().extension() == ".spv")
        {
            // .stem() is a filename without extension
            ShaderData& shaderData = m_ShaderData.Emplace(entry.path().stem().generic_string().c_str());

            DRE::ByteBuffer moduleBuffer{ static_cast<std::uint64_t>(entry.file_size()) };
            ReadFileToBuffer(entry.path().generic_string().c_str(), moduleBuffer);
            shaderData.m_Binary = DRE_MOVE(moduleBuffer);
            
            spirv_cross::Compiler compiler{ reinterpret_cast<std::uint32_t const*>(shaderData.m_Binary.Data()), shaderData.m_Binary.Size() / sizeof(std::uint32_t) };
            shaderData.m_ModuleType = SPVExecutionModelToVKWModuleType(compiler.get_execution_model());

            ParseShaderInterface(compiler, shaderData.m_Interface); 
        }
    }

    m_ShaderObserverThread = std::thread{ &IOManager::ShaderObserver, this };
}

void IOManager::ShaderObserver()
{
    //                                                                                                                     required for dirs
    HANDLE directoryHandle = CreateFileA("shaders", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (directoryHandle == INVALID_HANDLE_VALUE)
    {
        std::cout << "IOManager::ShaderObserver: Failed to create directory handle. Terminating thread." << std::endl;
        return;
    }

    DRE::Stopwatch stopwatch;

    while (true)
    {
        static std::uint8_t buffer[1024];
        std::uint8_t* bufferPtr = DRE::PtrAlign(buffer, sizeof(DWORD));
        DWORD bytesReturned = 0;
        if (ReadDirectoryChangesW(directoryHandle, bufferPtr, 1024, FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE, &bytesReturned, NULL, NULL) == 0)
        {
            std::cout << "IOManager::ShaderObserver: Failed to get directory changes." << std::endl;
        }
        
        // timeout to skip duplicate events
        if (stopwatch.CurrentSeconds() < 1)
        {
            continue;
        }

        FILE_NOTIFY_INFORMATION* infoPtr = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(bufferPtr);
        while (infoPtr != nullptr)
        {
            if (infoPtr->Action != FILE_ACTION_MODIFIED)
            {
                std::cout << "IOManager::ShaderObserver: Unsupported file event. Terminating thread." << std::endl;
                return;
            }

            char fileName[64];
            int const length = WideCharToMultiByte(CP_UTF8, 0, infoPtr->FileName, infoPtr->FileNameLength / sizeof(WCHAR), fileName, 64, NULL, NULL);
            if (length == 0)
            {
                std::cout << "IOManager::ShaderObserver: Failed to get ASCII file name from the event." << std::endl;
            }
            fileName[length] = '\0';

            char* spvExtStart = std::strrchr(fileName, '.');
            if (spvExtStart == nullptr) // file with no extension
            {
                infoPtr = infoPtr->NextEntryOffset == 0 ? nullptr : DRE::PtrAdd(infoPtr, infoPtr->NextEntryOffset);
                continue;
            }

            if (std::strcmp(spvExtStart + 1, "spv") != 0) // not a SPIR-V file
            {
                infoPtr = infoPtr->NextEntryOffset == 0 ? nullptr : DRE::PtrAdd(infoPtr, infoPtr->NextEntryOffset);
                continue;
            }

            std::cout << "IOManager::ShaderObserver: change in SPIR-V file " << fileName << " detected." << std::endl;

            char stem[64];
            char* stemEnd = std::strchr(fileName, '.');
            std::uint32_t stemSize = DRE::PtrDifference(stemEnd, fileName);
            std::strncpy(stem, fileName, stemSize);

            stem[stemSize] = '\0';
            {
                std::lock_guard guard{ m_PendingShadersMutex };
                m_PendingShaders.EmplaceBack(stem);

                DRE::String64 shaderPath{ "shaders\\" };
                shaderPath.Append(fileName, static_cast<DRE::U16>(DRE::PtrDifference(spvExtStart, fileName)));

                DRE::ByteBuffer& shaderBinary = m_ShaderData[fileName].m_Binary;
                ReadFileToBuffer(shaderPath, shaderBinary);
            }

            infoPtr = infoPtr->NextEntryOffset == 0 ? nullptr : DRE::PtrAdd(infoPtr, infoPtr->NextEntryOffset);
            stopwatch.Reset();
            m_PendingChangesFlag.store(true, std::memory_order::release);
        }
    }
}


}
