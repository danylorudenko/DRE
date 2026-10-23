#pragma once

#include <engine\io\ShaderDBImpl.hpp>
#include <engine\io\ShaderDB.hpp>

#include <engine\io\IOManager.hpp>
#include <foundation\system\Parallel.hpp>

#include <wrl\client.h>
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
        SlangResourceShape totalShape = type->getResourceShape();
        SlangResourceShape shape = SlangResourceShape(totalShape & SLANG_RESOURCE_BASE_SHAPE_MASK);
        DRE::U32 shapeExtFlags = totalShape & SLANG_RESOURCE_EXT_SHAPE_MASK;

        switch (shape)
        {
        case SlangResourceShape::SLANG_ACCELERATION_STRUCTURE:
            return VKW::DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE;
        case SlangResourceShape::SLANG_TEXTURE_1D:
        case SlangResourceShape::SLANG_TEXTURE_2D:
        case SlangResourceShape::SLANG_TEXTURE_3D:
        {
            if (type->getResourceAccess() == SlangResourceAccess::SLANG_RESOURCE_ACCESS_READ)
            {
                if (shapeExtFlags & SLANG_TEXTURE_COMBINED_FLAG)
                {
                    DRE_ASSERT(false, "ShaderDB: slang parser doesn't support combined image-samplers.");
                    return VKW::DESCRIPTOR_TYPE_NONE;
                }
                else
                {
                    return VKW::DESCRIPTOR_TYPE_TEXTURE;
                }
            }
            else
            {
                return VKW::DESCRIPTOR_TYPE_STORAGE_IMAGE;
            }
        }
        case SlangResourceShape::SLANG_STRUCTURED_BUFFER:
        {
            return VKW::DESCRIPTOR_TYPE_STORAGE_BUFFER;
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

void ParseShaderInterface(slang::ProgramLayout* layout, SlangStage& outStage, DRE::U32* workgroupSize, ShaderInterface& resultInterface)
{
    outStage = layout->getEntryPointByIndex(0)->getStage();
    if (outStage == SLANG_STAGE_COMPUTE)
    {
        slang::EntryPointReflection* entryPointReflection = layout->getEntryPointByIndex(0);
        DRE_ASSERT(entryPointReflection != nullptr, "Missing entry point reflection on compute shader.");

        SlangUInt wgSizes[3] = {};
        entryPointReflection->getComputeThreadGroupSize(3, wgSizes);

        workgroupSize[0] = static_cast<DRE::U32>(wgSizes[0]);
        workgroupSize[1] = static_cast<DRE::U32>(wgSizes[1]);
        workgroupSize[2] = static_cast<DRE::U32>(wgSizes[2]);
    }

    bool foundPushConstant = false;

    for (DRE::U32 i = 0, size = layout->getParameterCount(); i < size; i ++)
    {
        slang::VariableLayoutReflection* variableReflection = layout->getParameterByIndex(i);
        slang::TypeReflection* typeReflection = variableReflection->getType();

        if (variableReflection->getCategory() == slang::ParameterCategory::PushConstantBuffer)
        {
            DRE_ASSERT(!foundPushConstant, "Found second push constant. This is not supported.");
            foundPushConstant = true;
            slang::TypeLayoutReflection* typeLayoutReflection = variableReflection->getTypeLayout();
            if (typeLayoutReflection->getKind() == slang::TypeReflection::Kind::ConstantBuffer)
                typeLayoutReflection = typeLayoutReflection->getElementTypeLayout();

            DRE::SizeT const pushConstantSize = typeLayoutReflection->getSize(slang::ParameterCategory::Uniform);
            DRE_ASSERT(pushConstantSize < 128, "PushConstant is too large");

            resultInterface.m_PushConstantPresent = 1;
            resultInterface.m_PushConstantSize = pushConstantSize;
            resultInterface.m_PushConstantStages = SlangStageToVKWStage(outStage);

            continue;
        }

        DRE::U32    descriptorSet = variableReflection->getBindingSpace();
        DRE::U32    bindingIndex = variableReflection->getBindingIndex();
        char const* name = variableReflection->getName();

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
        member.stage = SlangStageToVKWStage(outStage);
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

    m_ShaderObserverThread = std::thread{ &ShaderDBImpl::ShaderObserver_Thread, this };
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

VKW::ShaderModuleType SlangStageToVKWModuleType(SlangStage stage)
{
    switch (stage)
    {
    case SLANG_STAGE_VERTEX:
        return VKW::SHADER_MODULE_TYPE_VERTEX;
    case SLANG_STAGE_PIXEL:
        return VKW::SHADER_MODULE_TYPE_FRAGMENT;
    case SLANG_STAGE_COMPUTE:
        return VKW::SHADER_MODULE_TYPE_COMPUTE;
    default:
        DRE_ASSERT(false, "Unknown stage of the resource");
        return VKW::SHADER_MODULE_TYPE_NONE;
    }
}

bool ShaderDBImpl::CompileShaderFile(DRE::String128 const& path)
{
    std::filesystem::path filePath{ path.GetData() };
    DRE::String128 fileName { filePath.filename().string().c_str() };

    std::cout << "Compiling shader " << path << std::endl;

    DRE::ByteBuffer sourceBlob{};
    std::uint64_t const bytesRead = m_IOManager->ReadFileStringToBuffer(filePath.generic_string().c_str(), &sourceBlob);
    DRE_ASSERT(bytesRead != 0, "Failed to read slang source.");

    ShaderFile& shaderFile = m_ShaderFileMap[fileName];
    shaderFile.fileName = fileName;
    shaderFile.path = filePath.generic_string().c_str();

    //slang::PreprocessorMacroDesc shaderTypeMacro;
    //shaderTypeMacro.name = GetShaderTypeDefineString(type);
    //shaderTypeMacro.value = "1";

    DRE::InplaceVector<slang::CompilerOptionEntry, 3> optionEntries;
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
    {
        auto& entry = optionEntries.EmplaceBack();
        entry.name = slang::CompilerOptionName::VulkanUseEntryPointName;
        entry.value.kind = slang::CompilerOptionValueKind::Int;
        entry.value.intValue0 = 1;
        entry.value.intValue1 = 0;
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
    //sessionDesc.preprocessorMacroCount = 1;
    //sessionDesc.preprocessorMacros = &shaderTypeMacro;
    sessionDesc.preprocessorMacroCount = 0;
    sessionDesc.preprocessorMacros = nullptr;
    //sessionDesc.structureSize // not needed
    Slang::ComPtr<slang::ISession> slangCurrentSession;
    m_SlangGlobalSession->createSession(sessionDesc, slangCurrentSession.writeRef());

    slang::IModule* slangModule = nullptr;
    {
        Slang::ComPtr<slang::IBlob> diagnosticBlob;
        
        slangModule = slangCurrentSession->loadModuleFromSourceString(fileName, nullptr, sourceBlob.As<char const*>(), diagnosticBlob.writeRef());
        if (diagnosticBlob != nullptr)
        {
            std::cout << (const char*)diagnosticBlob->getBufferPointer() << std::endl;
        }

        if (slangModule == nullptr)
        {
            std::cout << "Failed to load slang file:" << fileName << std::endl;
            return false;
        }
    }

    SlangInt32 entryPointCount = slangModule->getDefinedEntryPointCount();

    if (entryPointCount == 0)
    {
        std::cout << "Didn't find any entry points in slang file " << fileName << std::endl;
        return true;
    }

    bool allSuccess = true;
    for (SlangInt32 entryPointId = 0; entryPointId < entryPointCount; entryPointId++)
    {
        Slang::ComPtr<slang::IEntryPoint> entryPoint;
        slangModule->getDefinedEntryPoint(entryPointId, entryPoint.writeRef());
        if (entryPoint == nullptr)
        {
            std::cout << "Failed to get slang entry point " << entryPointId << " for file " << fileName << std::endl;
            allSuccess = false;
            continue;
        }

        char const* entryPointName = entryPoint->getFunctionReflection()->getName();

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

        if (composedProgram.get() == nullptr)
        {
            std::cout << "Slang failed to compose a program for file " << fileName << " entry point " << entryPointName << std::endl;
            allSuccess = false;
            continue;
        }

        slang::ProgramLayout* programLayout = nullptr;
        SlangStage shaderStage = SLANG_STAGE_COUNT;
        DRE::U32 workgroupSize[3] = { 0, 0, 0 };
        ShaderInterface resultInterface;
        {
            Slang::ComPtr<slang::IBlob> diagnosticsBlob;
            programLayout = composedProgram->getLayout(0, diagnosticsBlob.writeRef());

            if (programLayout == nullptr)
            {
                std::cout << "Slang failed to get program layout for file " << fileName << " entry point " << entryPointName << std::endl;
                allSuccess = false;
                continue;
            }

            ParseShaderInterface(programLayout, shaderStage, workgroupSize, resultInterface);
        }

        Slang::ComPtr<slang::IBlob> slangSpirv;
        {
            Slang::ComPtr<slang::IBlob> diagnosticBlob;
            SlangResult result = composedProgram->getEntryPointCode(0, 0, slangSpirv.writeRef(), diagnosticBlob.writeRef());

            if (diagnosticBlob != nullptr)
            {
                std::cout << (const char*)diagnosticBlob->getBufferPointer() << std::endl;
            }

            if (result != 0)
            {
                std::cout << "Slang unknown error when compiling spirv for file " << fileName << " entry point " << entryPointName << std::endl;
                allSuccess = false;
                continue;
            }
        }

        if (slangSpirv != nullptr && slangSpirv->getBufferSize() != 0)
        {
            DRE::ByteBuffer spvBuffer{ slangSpirv->getBufferSize() };
            DRE::MemCpy(spvBuffer.Data(), slangSpirv->getBufferPointer(), spvBuffer.Size());

            DRE::String64 entryName{ fileName };
            entryName.Append("_");
            entryName.Append(entryPointName);

            // we force clear the vector inside so we can copy into it later
            m_ShaderEntryMap[entryName].bindingInterface.m_Members.Clear();

            m_ShaderEntryMap[entryName] = ShaderEntry{
                .name = entryName,
                .entryPoint = entryPointName,
                .type = SlangStageToVKWModuleType(shaderStage),
                .workgroupSize = { workgroupSize[0], workgroupSize[1], workgroupSize[2] },
                .spirv = DRE_MOVE(spvBuffer),
                .source = sourceBlob,
                .bindingInterface = resultInterface
            };

            shaderFile.shaderEntries.EmplaceBackUnique(entryName);
            m_ShaderToFileMap[entryName] = fileName;

            std::cout << "Successfully compiled file " << fileName << ", entry point " << entryPointName << std::endl;
        }
        else
        {
            std::cout << "Slang failed to extract compilation result object for file " << fileName << ", entry point " << entryPointName << std::endl;
            allSuccess = false;
        }
    }

    return allSuccess;
}

ShaderEntry const* ShaderDBImpl::GetShaderEntry(DRE::String128 const& name)
{
    //m_ShaderEntryMap.ForEach([](auto const& pair)
    //    {
    //        std::cout << *pair.key << std::endl;
    //    });
    auto result = m_ShaderEntryMap.Find(name);
    return result.value;
}

ShaderFile const* ShaderDBImpl::GetShaderFile(DRE::String128 const& name)
{
    auto result = m_ShaderFileMap.Find(name);
    return result.value;
}

DRE::String128 ShaderDBImpl::BuildShaderPath(char const* shaderFile)
{
    DRE::String128 result { "shaders/" };
    result.Append(shaderFile);
    return result;
}

void ShaderDBImpl::CompileSources(bool parallel)
{
    struct ShaderFile
    {
        DRE::String128 name;
    };

    std::filesystem::recursive_directory_iterator dir_iterator{ "shaders", std::filesystem::directory_options::follow_directory_symlink };
    DRE::Vector<ShaderFile, DRE::AllocatorLinear> fileNames{ &DRE::g_FrameScratchAllocator };
    
    for (auto const& entry : dir_iterator)
    {
        if (entry.path().has_extension())
        {
            if (entry.path().extension() == ".slang")
            {
                fileNames.EmplaceBack(entry.path().generic_string().c_str());
            }
        }
    }

    auto compileTasks = DRE::ParallelFor<8>(fileNames.Size(), [this, &fileNames](DRE::U32 index)
    {
        DRE::String128& name = fileNames[index].name;
        bool success = CompileShaderFile(name.GetData());
        DRE_ASSERT(success, "Can't run with invalid shader.");
    }, parallel);

    compileTasks.Wait();
}

DRE::InplaceVector<DRE::String128, 12> ShaderDBImpl::ReadAndClearPendingShaderFilesCopy()
{
    std::lock_guard guard{ m_PendingShaderFilesMutex };
    m_PendingChangesFlag.store(false, std::memory_order::relaxed);
    return DRE_MOVE(m_PendingShaderFiles);
}

void ShaderDBImpl::ClearPendingShaders()
{
    std::lock_guard guard{ m_PendingShaderFilesMutex };
    m_PendingChangesFlag.store(false, std::memory_order_relaxed);
    m_PendingShaderFiles.Clear();
}

void ShaderDBImpl::ShaderObserver_Thread()
{
    //                                                                                                                     required for dirs
    HANDLE directoryHandle = CreateFileA("shaders", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (directoryHandle == INVALID_HANDLE_VALUE)
    {
        std::cout << "ShaderDB::ShaderObserver: Failed to create directory handle. Terminating thread." << std::endl;
        return;
    }

    while (true)
    {
        static constexpr DRE::U32 BUFFER_SIZE = 4096;

        static DRE::U8 buffer[BUFFER_SIZE];
        DRE::U8* bufferPtr = DRE::PtrAlign(buffer, sizeof(DWORD));
        DWORD bytesReturned = 0;
        if (ReadDirectoryChangesW(directoryHandle, bufferPtr, BUFFER_SIZE, TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE, &bytesReturned, NULL, NULL) == 0)
        {
            std::cout << "ShaderDB::ShaderObserver: Failed to get directory changes." << std::endl;
        }
        
        FILE_NOTIFY_INFORMATION* infoPtr = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(bufferPtr);
        while (infoPtr != nullptr)
        {
            if (infoPtr->Action != FILE_ACTION_MODIFIED)
            {
                std::cout << "ShaderDB::ShaderObserver: Unsupported file event. Terminating thread." << std::endl;
                return;
            }

            char fileName[128];
            int const length = WideCharToMultiByte(CP_UTF8, 0, infoPtr->FileName, infoPtr->FileNameLength / sizeof(WCHAR), fileName, 128, NULL, NULL);
            if (length == 0)
            {
                std::cout << "ShaderDB::ShaderObserver: Failed to get ASCII file name from the event." << std::endl;
            }
            fileName[length] = '\0';

            char* ExtStart = std::strrchr(fileName, '.');
            if (ExtStart == nullptr || 
                (std::strcmp(ExtStart, ".slang") != 0))
            {
                infoPtr = infoPtr->NextEntryOffset == 0 ? nullptr : DRE::PtrAdd(infoPtr, infoPtr->NextEntryOffset);
                continue;
            }

            char* FileNameStart = std::strrchr(fileName, '\\');
            if (FileNameStart != nullptr)
            {
                FileNameStart = FileNameStart + 1;
            }
            else
            {
                FileNameStart = fileName;
            }

            DRE::String128 fileNameStr{ "" };
            fileNameStr.Append(FileNameStart);

            auto Pair = m_ShaderFileMap.Find(fileNameStr);
            if (Pair.key == nullptr)
            {
                infoPtr = infoPtr->NextEntryOffset == 0 ? nullptr : DRE::PtrAdd(infoPtr, infoPtr->NextEntryOffset);
                continue;
            }

            {
                std::lock_guard guard{ m_PendingShaderFilesMutex };
                if (m_PendingShaderFiles.Find(fileNameStr) == m_PendingShaderFiles.Size())
                {
                    std::cout << "ShaderDB::ShaderObserver: Found new change in shader file: " << fileNameStr.GetData() << std::endl;
                    m_PendingShaderFiles.EmplaceBack(fileNameStr);
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