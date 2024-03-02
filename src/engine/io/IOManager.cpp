#include <engine\io\IOManager.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <utility>
#include <charconv>
#include <filesystem>
#include <future>

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
#include <shaderc\shaderc.hpp>

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

struct DREIncludeData
{
    DRE::ByteBuffer content;
    DRE::String64 contentName;
    DRE::String64 targetName;
};

class DREIncluder : public shaderc::CompileOptions::IncluderInterface
{
public:
    DREIncluder(IOManager* manager)
        : m_IOManager{ manager }
    {}

    virtual shaderc_include_result* GetInclude(const char* requested_source,
        shaderc_include_type type,
        const char* requesting_source,
        size_t include_depth) override
    {
        std::mutex& m = m_IOManager->GetShaderIncluderMutex();
        m.lock();
        shaderc_include_result* result = DRE::g_FrameScratchAllocator.Alloc<shaderc_include_result>();
        DREIncludeData* data = new (DRE::g_FrameScratchAllocator.Alloc<DREIncludeData>()) DREIncludeData{};
        m.unlock();

        data->contentName = "shaders\\";
        data->contentName.Append(requested_source);

        data->targetName = requesting_source;
        
        std::uint64_t const bytesRead = IOManager::ReadFileToBuffer(data->contentName.GetData(), &data->content);
        DRE_ASSERT(bytesRead != 0, "Failed to read requested include for GLSL.");

        result->source_name = data->contentName.GetData();
        result->content_length = data->content.Size();
        result->content = data->content.As<char*>();
        result->source_name = data->targetName.GetData();
        result->source_name_length = data->targetName.GetSize();
        result->user_data = data;

        return result;
    }

    // Handles shaderc_include_result_release_fn callbacks.
    virtual void ReleaseInclude(shaderc_include_result* result) override
    {
        DREIncludeData* data = (DREIncludeData*)result->user_data;
        data->~DREIncludeData();
    }

    virtual ~DREIncluder() = default;

private:
    IOManager* m_IOManager;
};

DRE::ByteBuffer IOManager::CompileGLSL(char const* path)
{
    std::filesystem::path filePath{ path };

    std::cout << "Compiling shader " << path << std::endl;

    shaderc_shader_kind kind = (shaderc_shader_kind)0;

    auto extension = filePath.extension();
    if (extension == ".vert")
        kind = shaderc_glsl_vertex_shader;
    else if (extension == ".frag")
        kind = shaderc_glsl_fragment_shader;
    else if (extension == ".comp")
        kind = shaderc_glsl_compute_shader;
    else
        DRE_ASSERT(false, "Attempt to compile unsupported shader type. See file extension.");

    DRE::ByteBuffer sourceBlob{};
    std::uint64_t const bytesRead = ReadFileToBuffer(path, &sourceBlob);
    DRE_ASSERT(bytesRead != 0, "Failed to read GLSL source.");

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env::shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
    options.SetIncluder(std::make_unique<DREIncluder>(this));

    shaderc::PreprocessedSourceCompilationResult preprocess = compiler.PreprocessGlsl(sourceBlob.As<char*>(), sourceBlob.Size(), kind, path, options);
    if (preprocess.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        std::cout << preprocess.GetErrorMessage();
        return DRE::ByteBuffer{};
    }

    DRE::U64 const preprocessedSize = DRE::PtrDifference(preprocess.end(), preprocess.begin());
    shaderc::SpvCompilationResult compile = compiler.CompileGlslToSpv(preprocess.begin(), preprocessedSize, kind, path, "main", options);

    if (compile.GetCompilationStatus() != shaderc_compilation_status_success) 
    {
        std::cout << compile.GetErrorMessage();
        return DRE::ByteBuffer{};
    }

    DRE::U64 const moduleSize = DRE::PtrDifference(compile.end(), compile.begin());
    return DRE::ByteBuffer{ (void*)compile.begin(), moduleSize };
}

DRE::InplaceVector<DRE::String64, 12> IOManager::GetPendingShaders()
{
    std::lock_guard guard{ m_PendingShadersMutex };
    IOManager::m_PendingChangesFlag.store(false, std::memory_order::relaxed);
    return DRE_MOVE(m_PendingShaders);
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
        material->AssignTextureToSlot(slot, DRE_MOVE(dataTexture));
    }
    else
    {
        // process aiTexture
        // fuck this for now
        DRE_ASSERT(false, "assimp embedded textures are not yet supported");
    }
}

void IOManager::ParseAssimpNodeRecursive(VKW::Context& gfxContext, char const* assetPath, aiScene const* scene, char const* sceneName, aiNode const* node, WORLD::Scene& targetScene, WORLD::SceneNode* parentNode)
{
    aiMatrix4x4 const t = node->mTransformation;
    glm::mat4 const transform {
            t.a1, t.a2, t.a3, t.a4,
            t.b1, t.b2, t.b3, t.b4,
            t.c1, t.c2, t.c3, t.c4,
            t.d1, t.d2, t.d3, t.d4
    };

    WORLD::SceneNode* aggregatorNode = targetScene.CreateEmptySceneNode(parentNode);
    aggregatorNode->SetMatrix(transform);

    for (std::uint32_t i = 0, count = node->mNumMeshes; i < count; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

        Data::Material* material = m_MaterialLibrary->GetMaterial(mesh->mMaterialIndex, sceneName);
        Data::Geometry* geometry = m_GeometryLibrary->GetGeometry(node->mMeshes[i], sceneName); // this is the main problem:
        // assimp references (and we too) meshes by integers which are not globally unique.
        WORLD::Entity* entity = targetScene.CreateOpaqueEntity(gfxContext, geometry, material, aggregatorNode);
    }

    for (std::uint32_t i = 0; i < node->mNumChildren; i++)
    {
        ParseAssimpNodeRecursive(gfxContext, assetPath, scene, sceneName, node->mChildren[i], targetScene, aggregatorNode);
    }
}

void IOManager::ParseModelFile(char const* path, WORLD::Scene& targetScene, char const* defaultShader, glm::mat4 parentTransform, Data::TextureChannelVariations metalnessRoughnessOverride)
{
    Assimp::Importer importer = Assimp::Importer();

    aiScene const* scene = importer.ReadFile(path, aiProcessPreset_TargetRealtime_Fast | aiProcess_FlipUVs);
    char const* sceneName = aiScene::GetShortFilename(path);

    DRE_ASSERT(scene != nullptr, "Failed to load a model file.");
    if (scene == nullptr)
        return;

    ParseAssimpMeshes(GFX::g_GraphicsManager->GetMainContext(), scene, sceneName);
    ParseAssimpMaterials(scene, sceneName, path, defaultShader, metalnessRoughnessOverride);

    WORLD::SceneNode* parentNode = targetScene.CreateEmptySceneNode();
    parentNode->SetMatrix(parentTransform);

    ParseAssimpNodeRecursive(GFX::g_GraphicsManager->GetMainContext(), path, scene, sceneName, scene->mRootNode, targetScene, parentNode);
    GFX::g_GraphicsManager->GetMainContext().FlushAll();
}

void IOManager::ParseAssimpMaterials(aiScene const* scene, char const* sceneName, char const* path, char const* defaultShader, Data::TextureChannelVariations metalnessRoughnessOverride)
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
        material->GetRenderingProperties().SetShader(defaultShader);
    }
}

void IOManager::ParseAssimpMeshes(VKW::Context& gfxContext, aiScene const* scene, char const* sceneName)
{
    for (std::uint32_t i = 0, size = scene->mNumMeshes; i < size; i++)
    {
        aiMesh* mesh = scene->mMeshes[i];

        Data::Geometry geometry{ sizeof(Data::DREVertex), 4 };
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
            geometry.GetIndex<std::uint32_t>(j*3 + 0) = mesh->mFaces[j].mIndices[0];
            geometry.GetIndex<std::uint32_t>(j*3 + 1) = mesh->mFaces[j].mIndices[1];
            geometry.GetIndex<std::uint32_t>(j*3 + 2) = mesh->mFaces[j].mIndices[2];
        }

        m_GeometryLibrary->AddGeometry(i, sceneName, DRE_MOVE(geometry));
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
        const spirv_cross::SPIRType& type = compiler.get_type(res.base_type_id);
        resultInterface.m_PushConstantSize = compiler.get_declared_struct_size(type);
        resultInterface.m_PushConstantPresent = 1;
        resultInterface.m_PushConstantStages = SPVExecutionModelToVKWStage(compiler.get_execution_model());
    }
}

void IOManager::CompileGLSLSources()
{
    std::filesystem::recursive_directory_iterator dir_iterator{ "shaders", std::filesystem::directory_options::follow_directory_symlink };
    DRE::Vector<DRE::String64, DRE::AllocatorLinear> fileNames{ &DRE::g_FrameScratchAllocator };
    
    for (auto const& entry : dir_iterator)
    {
        if (entry.path().has_extension() &&
            (entry.path().extension() == ".vert" || entry.path().extension() == ".frag" || entry.path().extension() == ".comp"))
        {
            fileNames.EmplaceBack(entry.path().generic_string().c_str());
        }
    }

    std::uint32_t constexpr parallelFactor = 4;
    std::uint32_t const parallelChunkSize = fileNames.Size() / parallelFactor + 1;

    DRE::InplaceVector<std::future<void>, parallelFactor> parallelCompilations;
    for (std::uint32_t i = 0; i < parallelFactor; i++)
    {
        parallelCompilations.EmplaceBack(std::async(std::launch::async, [parallelChunkSize, &parallelCompilations, &fileNames, this](std::uint32_t chunkID) 
            {
                std::uint32_t chunkStart = chunkID * parallelChunkSize;
                std::uint32_t chunkEnd = DRE::Min((chunkID + 1) * parallelChunkSize, fileNames.Size());

                for (std::uint32_t j = chunkStart; j < chunkEnd; j++)
                {
                    DRE::String64& name = fileNames[j];
                    DRE::ByteBuffer spirv = CompileGLSL(name.GetData());
                    DRE_ASSERT(spirv.Size() > 0, "Can't run with invalid shader.");
                    name.Append(".spv");
                    WriteNewFile(name.GetData(), spirv);
                }
            }, i));
    }

    // std::wait_all not available yet/experimental. Though we should be fine
    for (std::uint32_t i = 0; i < parallelFactor; i++)
    {
        parallelCompilations[i].wait();
    }
}

void IOManager::LoadShaderBinaries()
{
    std::filesystem::recursive_directory_iterator dir_iterator{ "shaders", std::filesystem::directory_options::follow_directory_symlink };

    for (auto const& entry : dir_iterator)
    {
        if (entry.path().has_extension() && entry.path().extension() == ".spv")
        {
            // .stem() is a filename without extension
            ShaderData& shaderData = m_ShaderData.Emplace(entry.path().stem().generic_string().c_str());

            DRE::ByteBuffer moduleBuffer{ static_cast<std::uint64_t>(entry.file_size()) };
            ReadFileToBuffer(entry.path().generic_string().c_str(), &moduleBuffer);
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

    while (true)
    {
        static std::uint8_t buffer[1024];
        std::uint8_t* bufferPtr = DRE::PtrAlign(buffer, sizeof(DWORD));
        DWORD bytesReturned = 0;
        if (ReadDirectoryChangesW(directoryHandle, bufferPtr, 1024, FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE, &bytesReturned, NULL, NULL) == 0)
        {
            std::cout << "IOManager::ShaderObserver: Failed to get directory changes." << std::endl;
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

            char* ExtStart = std::strrchr(fileName, '.');
            if (ExtStart == nullptr || 
                (std::strcmp(ExtStart, ".vert") != 0 &&
                std::strcmp(ExtStart, ".frag") != 0 &&
                std::strcmp(ExtStart, ".comp") != 0))
            {
                infoPtr = infoPtr->NextEntryOffset == 0 ? nullptr : DRE::PtrAdd(infoPtr, infoPtr->NextEntryOffset);
                continue;
            }

            DRE::String64 stem{ fileName };
            char* stemEnd = std::strchr(fileName, '.');
            stem.Shrink(DRE::PtrDifference(stemEnd, fileName));

            {
                std::lock_guard guard{ m_PendingShadersMutex };
                if (m_PendingShaders.Find(stem) == m_PendingShaders.Size())
                {
                    m_PendingShaders.EmplaceBack(stem);
                }
                m_PendingChangesFlag.store(true, std::memory_order::release);
            }

            infoPtr = infoPtr->NextEntryOffset == 0 ? nullptr : DRE::PtrAdd(infoPtr, infoPtr->NextEntryOffset);
        }
    }
}


}
