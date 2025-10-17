#pragma once

#include <engine\io\ShaderDBImpl.hpp>
#include <engine\io\ShaderDB.hpp>

#include <engine\io\IOManager.hpp>

#include <spirv_cross.hpp>
#include <wrl/client.h>
#include <dxcapi.h>

#include <slang.h>
#include <slang-com-ptr.h>
#include <slang-com-helper.h>
#include <slang-gfx.h>

#include <iostream>
#include <filesystem>
#include <future>

namespace IO
{

class IOManager;

/*
// keep this object until hlsl was compiled
/////////////////////////////////
// DXCArgsBuilder
DXCArgsBuilder::DXCArgsBuilder()
    : m_GenericArgsBuffer{}
    , m_GenericArgsBufferOffset{ 0 }
{
    m_CompilationArgs.EmplaceBack(DXC_ARG_PACK_MATRIX_COLUMN_MAJOR);
    m_CompilationArgs.EmplaceBack(DXC_ARG_WARNINGS_ARE_ERRORS);
    m_CompilationArgs.EmplaceBack(L"-spirv");
    m_CompilationArgs.EmplaceBack(L"-fspv-target-env=vulkan1.3");
    m_CompilationArgs.EmplaceBack(L"-fspv-preserve-bindings");
    m_CompilationArgs.EmplaceBack(L"-fspv-preserve-interface");
    m_CompilationArgs.EmplaceBack(L"-fspv-reflect");
    m_CompilationArgs.EmplaceBack(L"-Ishaders");
    m_CompilationArgs.EmplaceBack(m_GenericArgsBuffer); // filled later

    //m_CompilationArgs.EmplaceBack(L"-fvk-use-dx-position-w");
    //m_CompilationArgs.EmplaceBack(L"-fvk-invert-y");
}

void DXCArgsBuilder::AddDebugArgs()
{
    m_CompilationArgs.EmplaceBack(L"-O0");
    m_CompilationArgs.EmplaceBack(L"-Zi");
    m_CompilationArgs.EmplaceBack(L"-fspv-debug=vulkan-with-source");
}

void DXCArgsBuilder::AddOptimizedArgs()
{
    m_CompilationArgs.EmplaceBack(L"-O3");
}

void DXCArgsBuilder::AddEntryPoint(char const* entryPoint, VKW::ShaderModuleType type)
{
    //AddGenericArgument("-fspv-entrypoint-name=", entryPoint);
    AddGenericArgument("-E", entryPoint);
    AddProfileArgs(type);
}

void DXCArgsBuilder::AppendGenericArgument(char const* input)
{
    DRE::SizeT size = std::strlen(input);
    PeekGenericBufferSize(size);

    std::mbstate_t state{};

    DRE::SizeT result = 0;
    std::mbsrtowcs(
        m_GenericArgsBuffer + m_GenericArgsBufferOffset,
        &input,
        GENERIC_ARGUMENT_BUFFER_SIZE - m_GenericArgsBufferOffset,
        &state);

    m_GenericArgsBufferOffset += size;

}

void DXCArgsBuilder::AddGenericArgument(char const* prefix, char const* argument)
{
    AppendGenericArgument(prefix);
    AppendGenericArgument(argument);
}

void DXCArgsBuilder::PeekGenericBufferSize(DRE::U32 appendSize)
{
    DRE_ASSERT((m_GenericArgsBufferOffset + appendSize) < (GENERIC_ARGUMENT_BUFFER_SIZE - 1), "dxc define buffer overflow. See GENERIC_ARGUMENT_BUFFER_SIZE");
}

void DXCArgsBuilder::AddProfileArgs(VKW::ShaderModuleType type)
{
    wchar_t const* profileStr = nullptr;
    switch (type)
    {
    case VKW::SHADER_MODULE_TYPE_VERTEX:
        profileStr = L"-Tvs_6_0";
        m_CompilationArgs.EmplaceBack(L"-DDRE_VERTEX_SHADER=1");
        break;
    case VKW::SHADER_MODULE_TYPE_FRAGMENT:
        profileStr = L"-Tps_6_0";
        m_CompilationArgs.EmplaceBack(L"-DDRE_PIXEL_SHADER=1");
        break;
    case VKW::SHADER_MODULE_TYPE_COMPUTE:
        profileStr = L"-Tcs_6_0";
        m_CompilationArgs.EmplaceBack(L"-DDRE_COMPUTE_SHADER=1");
        break;
    default:
        DRE_ASSERT(false, "DXCArgsBuilder: shader type not supported");
    };

    m_CompilationArgs.EmplaceBack(profileStr);
}
*/

/*
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

template<typename TResourceArray>
void ParseShaderInterfaceType(spirv_cross::Compiler& compiler, TResourceArray const& resources, VKW::DescriptorType type, ShaderInterface& resultInterface)
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
*/

VKW::DescriptorStage SlangStageToVKWStage(SlangStage stage)
{
    switch (stage)
    {
    case SLANG_STAGE_VERTEX:
        return VKW::DESCRIPTOR_STAGE_VERTEX;
    case SLANG_STAGE_PIXEL:
        return VKW::DESCRIPTOR_STAGE_FRAGMENT;
    case SLANG_STAGE_COMPUTE:
        return VKW::DESCRIPTOR_STAGE_COMPUTE;
    default:
        DRE_ASSERT(false, "Unknown stage of the resource");
        return VKW::DESCRIPTOR_STAGE_NONE;
    }
}

VKW::DescriptorType SlangTypeToDescriptorArraylessType(slang::TypeReflection* type)
{
    slang::TypeReflection* actualType = type;
    if (actualType->getKind() == slang::TypeReflection::Kind::Array)
    {
        actualType = actualType->getElementType();
        DRE_ASSERT(actualType->getKind() != slang::TypeReflection::Kind::Array, "We don't support multidimentional arrays in shaders");
    }

    switch (actualType->getKind())
    {
    case slang::TypeReflection::Kind::ConstantBuffer:
        return VKW::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        break;
    case slang::TypeReflection::Kind::Resource:
    {
        switch (type->getResourceShape())
        {
        case SlangResourceShape::SLANG_ACCELERATION_STRUCTURE:
            return VKW::DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE;
        case SlangResourceShape::SLANG_TEXTURE_1D:
        case SlangResourceShape::SLANG_TEXTURE_2D:
        case SlangResourceShape::SLANG_TEXTURE_3D:
        {
            if (type->getResourceAccess() == SlangResourceAccess::SLANG_RESOURCE_ACCESS_READ)
                return VKW::DESCRIPTOR_TYPE_TEXTURE;
            else
                return VKW::DESCRIPTOR_TYPE_STORAGE_IMAGE;
        }
        default:
            DRE_ASSERT(false, "Unknown resource shape in the slang encountered.");
            return VKW::DESCRIPTOR_TYPE_NONE;
        }
    }
        break;
    case slang::TypeReflection::Kind::SamplerState:
        return VKW::DESCRIPTOR_TYPE_SAMPLER;
    case slang::TypeReflection::Kind::ShaderStorageBuffer:
        return VKW::DESCRIPTOR_TYPE_STORAGE_BUFFER;
    default:
        DRE_ASSERT(false, "Unsupported descriptor type found in shader");
        return VKW::DESCRIPTOR_TYPE_NONE;
    }
}

void ParseShaderInterface(slang::ProgramLayout* layout, ShaderInterface& resultInterface)
{
    for (DRE::U32 i = 0, size = layout->getParameterCount(); i < size; i ++)
    {
        slang::VariableLayoutReflection* variableReflection = layout->getParameterByIndex(i);
        DRE::U32    descriptorSet = variableReflection->getBindingSpace();
        DRE::U32    bindingIndex = variableReflection->getBindingIndex();
        char const* name = variableReflection->getName();
        SlangStage  stage = layout->getEntryPointByIndex(0)->getStage();

        slang::TypeReflection* typeReflection = variableReflection->getType();
        slang::TypeReflection::Kind typeKind = typeReflection->getKind();

        DRE::U64 elementCount = 1;
        if (typeKind == slang::TypeReflection::Kind::Array)
        {
            elementCount = typeReflection->getElementCount();
            if (elementCount == ~size_t(0))
                elementCount = 0; // we mark unbound arrays with 0
        }

        VKW::DescriptorType vkwType = SlangTypeToDescriptorArraylessType(typeReflection);

        auto& member = resultInterface.m_Members.EmplaceBack();
        member.type = vkwType;
        member.stage = SlangStageToVKWStage(stage);
        member.set = descriptorSet;
        member.binding = bindingIndex;
        member.arraySize = elementCount;
    }
}


/////////////////////////////////
// ShaderModuleDB
#define COM_CHECK(expr) \
{\
    HRESULT res = expr;\
    if (FAILED(res)) {\
        std::cerr << "HRESULT:" << res << std::endl;\
        DRE_ASSERT(false, "Failed DXC call: " #expr);\
    }\
}
ShaderDBImpl::ShaderDBImpl(IO::IOManager* io)
    : m_IOManager{ io }
    , m_PendingChangesFlag{ false }
{
    {
        SlangGlobalSessionDesc globalSessionDesc = {};
        slang::createGlobalSession(&globalSessionDesc, m_SlangGlobalSession.writeRef());
        DRE_ASSERT(m_SlangGlobalSession.get() != nullptr, "Failed to create Slang Global Session");
    }

    m_ShaderObserverThread = std::thread{ &ShaderDBImpl::ShaderObserver, this };
}

ShaderDBImpl::~ShaderDBImpl()
{
    m_ShaderObserverThread.detach();
}

char const* GetShaderTypeDefineString(VKW::ShaderModuleType type)
{
    switch (type)
    {
    case VKW::SHADER_MODULE_TYPE_VERTEX:
        return "DRE_VERTEX_SHADER";
    case VKW::SHADER_MODULE_TYPE_FRAGMENT:
        return "DRE_PIXEL_SHADER";
    case VKW::SHADER_MODULE_TYPE_COMPUTE:
        return "DRE_COMPUTE_SHADER";
    default:
        DRE_ASSERT(false, "DXCArgsBuilder: shader type not supported");
        return "";
    };
}

bool ShaderDBImpl::CompileShader(DRE::String64 const& path, VKW::ShaderModuleType type)
{
    std::filesystem::path filePath{ path.GetData() };
    DRE::String64 name { filePath.filename().string().c_str() };

    std::cout << "Compiling shader " << path << std::endl;

    DRE::ByteBuffer sourceBlob{};
    std::uint64_t const bytesRead = m_IOManager->ReadFileStringToBuffer(filePath.generic_string().c_str(), &sourceBlob);
    DRE_ASSERT(bytesRead != 0, "Failed to read GLSL source.");

    slang::PreprocessorMacroDesc shaderTypeMacro;
    shaderTypeMacro.name = GetShaderTypeDefineString(type);
    shaderTypeMacro.value = "1";

    DRE::InplaceVector<slang::CompilerOptionEntry, 2> optionEntries;
    {
        //auto& entry = optionEntries.EmplaceBack();
        //entry.name = slang::CompilerOptionName::Capability;
        //entry.value.intValue0 = slang::spvRayQuery
    }
    {
        auto& entry = optionEntries.EmplaceBack();
        entry.name = slang::CompilerOptionName::PreserveParameters;
        entry.value.kind = slang::CompilerOptionValueKind::Int;
        entry.value.intValue0 = 1;
        entry.value.intValue1 = 1;
    }

    slang::TargetDesc targetDesc;
    targetDesc.format = SlangCompileTarget::SLANG_SPIRV;
    targetDesc.flags = SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY; // set by default and will be deprecated, use CompilerOption instead.
    targetDesc.compilerOptionEntryCount = optionEntries.Size();
    targetDesc.compilerOptionEntries = optionEntries.Data();

    // from slangc --help: Accepted profiles are:
    //      *sm_{ 4_0,4_1,5_0,5_1,6_0,6_1,6_2,6_3,6_4,6_5,6_6 }
    //      *glsl_{ 110,120,130,140,150,330,400,410,420,430,440,450,460 }
    // But here we see spirv
    // https://github.com/shader-slang/slang/blob/master/examples/hello-world/main.cpp
    targetDesc.profile = m_SlangGlobalSession->findProfile("spirv_1_5");

    char const* paths = "shaders/";
    slang::SessionDesc sessionDesc;
    sessionDesc.targetCount = 1;
    sessionDesc.targets = &targetDesc;
    sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
    sessionDesc.searchPathCount = 1;
    sessionDesc.searchPaths = &paths;
    sessionDesc.preprocessorMacroCount = 1;
    sessionDesc.preprocessorMacros = &shaderTypeMacro;
    //sessionDesc.structureSize // not needed
    Slang::ComPtr<slang::ISession> slangCurrentSession;
    m_SlangGlobalSession->createSession(sessionDesc, slangCurrentSession.writeRef());

    slang::IModule* slangModule = nullptr;
    {
        Slang::ComPtr<slang::IBlob> diagnosticBlob;
        
        slangModule = slangCurrentSession->loadModuleFromSourceString(name, nullptr, sourceBlob.As<char const*>(), diagnosticBlob.writeRef());
        if (diagnosticBlob != nullptr)
        {
            std::cout << (const char*)diagnosticBlob->getBufferPointer() << std::endl;
        }
        DRE_ASSERT(slangModule != nullptr, "Failed to load a slang module");
    }

    Slang::ComPtr<slang::IEntryPoint> entryPoint;
    slangModule->findEntryPointByName("main", entryPoint.writeRef());

    DRE_ASSERT(entryPoint != nullptr, "Failed to find slang entry point \"main\"");

    DRE::InplaceVector<slang::IComponentType*, 2> slangComponents;
    slangComponents.EmplaceBack(slangModule);
    slangComponents.EmplaceBack(entryPoint.get());

    Slang::ComPtr<slang::IComponentType> composedProgram;
    {
        Slang::ComPtr<slang::IBlob> diagnosticBlob;
        slangCurrentSession->createCompositeComponentType(
            slangComponents.Data(),
            slangComponents.Size(),
            composedProgram.writeRef(),
            diagnosticBlob.writeRef()
        );

        if (diagnosticBlob != nullptr)
        {
            std::cout << (const char*)diagnosticBlob->getBufferPointer() << std::endl;
        }
    }

    slang::ProgramLayout* programLayout = nullptr;
    ShaderInterface resultInterface;
    {
        DRE_ASSERT(composedProgram.get() != nullptr, "Slang failed to compose a program.");

        Slang::ComPtr<slang::IBlob> diagnosticsBlob;
        programLayout = composedProgram->getLayout(0, diagnosticsBlob.writeRef());
        DRE_ASSERT(programLayout != nullptr, "Slang failed to get program layout");

        ParseShaderInterface(programLayout, resultInterface);
    }

    Slang::ComPtr<slang::IBlob> slangSpirv;
    {
        Slang::ComPtr<slang::IBlob> diagnosticBlob;
        SlangResult result = composedProgram->getEntryPointCode(0, 0, slangSpirv.writeRef(), diagnosticBlob.writeRef());

        if (diagnosticBlob != nullptr)
        {
            std::cout << (const char*)diagnosticBlob->getBufferPointer() << std::endl;
        }
        DRE_ASSERT(result == 0, "slang unknown error when compiling spirv");

    }

    if (slangSpirv != nullptr && slangSpirv->getBufferSize() != 0)
    {
        DRE::ByteBuffer spvBuffer{ slangSpirv->getBufferSize() };
        DRE::MemCpy(spvBuffer.Data(), slangSpirv->getBufferPointer(), spvBuffer.Size());

        // we force clear the vector inside so we can copy into it later
        m_ShaderMap[name].bindingInterface.m_Members.Clear();

        m_ShaderMap[name] = ShaderEntry{
            .name = name,
            .type = type,
            .spirv = DRE_MOVE(spvBuffer),
            .source = sourceBlob,
            .bindingInterface = resultInterface
        };

        return true;
    }
    else
    {
        DRE_ASSERT(false, "slang failed to extract compilation result object");
        return false;
    }
}

ShaderEntry const* ShaderDBImpl::GetShaderEntry(DRE::String64 const& name)
{
    auto result = m_ShaderMap.Find(name);
    return result.value;
}


void ShaderDBImpl::CompileSources(bool parallel)
{
    struct ShaderFile
    {
        DRE::String64 name; VKW::ShaderModuleType type;
    };

    std::filesystem::recursive_directory_iterator dir_iterator{ "shaders", std::filesystem::directory_options::follow_directory_symlink };
    DRE::Vector<ShaderFile, DRE::AllocatorLinear> fileNames{ &DRE::g_FrameScratchAllocator };
    
    for (auto const& entry : dir_iterator)
    {
        if (entry.path().has_extension())
        {

            if (entry.path().extension() == ".vert")
            {
                fileNames.EmplaceBack(entry.path().generic_string().c_str(), VKW::SHADER_MODULE_TYPE_VERTEX);
            }
            else if (entry.path().extension() == ".frag")
            {
                fileNames.EmplaceBack(entry.path().generic_string().c_str(), VKW::SHADER_MODULE_TYPE_FRAGMENT);
            }
            else if (entry.path().extension() == ".comp")
            {
                fileNames.EmplaceBack(entry.path().generic_string().c_str(), VKW::SHADER_MODULE_TYPE_COMPUTE);
            }
        }
    }

    std::uint32_t constexpr MAX_PARALLEL_FACTOR = 8;
    std::uint32_t const parallelFactor = parallel ? MAX_PARALLEL_FACTOR : 1;
    std::uint32_t const parallelChunkSize = fileNames.Size() / parallelFactor + 1;

    DRE::InplaceVector<std::future<void>, MAX_PARALLEL_FACTOR> parallelCompilations;
    for (std::uint32_t i = 0; i < parallelFactor; i++)
    {
        parallelCompilations.EmplaceBack(std::async(std::launch::async, [parallelChunkSize, &parallelCompilations, &fileNames, this](std::uint32_t chunkID) 
            {
                std::uint32_t chunkStart = chunkID * parallelChunkSize;
                std::uint32_t chunkEnd = DRE::Min((chunkID + 1) * parallelChunkSize, fileNames.Size());

                for (std::uint32_t j = chunkStart; j < chunkEnd; j++)
                {
                    DRE::String64& name = fileNames[j].name;
                    DRE::ByteBuffer spirv = CompileShader(name.GetData(), fileNames[j].type);
                    DRE_ASSERT(spirv.Size() > 0, "Can't run with invalid shader.");
                }
            }, i));
    }

    // std::wait_all not available yet/experimental. Though we should be fine
    for (std::uint32_t i = 0; i < parallelFactor; i++)
    {
        parallelCompilations[i].wait();
    }
}

DRE::InplaceVector<DRE::String64, 12> ShaderDBImpl::GetPendingShaders()
{
    std::lock_guard guard{ m_PendingShadersMutex };
    m_PendingChangesFlag.store(false, std::memory_order::relaxed);
    return DRE_MOVE(m_PendingShaders);
}


void ShaderDBImpl::ShaderObserver()
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

#undef COM_CHECK



void ShaderInterface::Merge(ShaderInterface const& rhs)
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

    m_PushConstantPresent |= rhs.m_PushConstantPresent;
    m_PushConstantSize = rhs.m_PushConstantSize > m_PushConstantSize ? rhs.m_PushConstantSize : m_PushConstantSize;
    m_PushConstantStages = static_cast<VKW::DescriptorStage>(static_cast<std::uint16_t>(rhs.m_PushConstantStages) | m_PushConstantStages);
}

bool ShaderInterface::Member::operator==(ShaderInterface::Member const& rhs) const
{
    return 
        (type == rhs.type) &&
        //(stage == rhs.stage) && this will differ, because we're merging different stages
        (set == rhs.set) &&
        (binding == rhs.binding) &&
        (arraySize == rhs.arraySize);
}

bool ShaderInterface::Member::operator!=(ShaderInterface::Member const& rhs) const
{
    return !operator==(rhs);
}

}