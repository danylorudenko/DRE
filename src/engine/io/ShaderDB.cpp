#pragma once

#include <engine\io\ShaderDB.hpp>

#include <engine\io\IOManager.hpp>

#include <spirv_cross.hpp>
#include <wrl/client.h>
#include <dxcapi.h>

#include <iostream>

namespace IO
{

class IOManager;

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
{
    //CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    COM_CHECK(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_Utils)));
    COM_CHECK(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_Compiler)));
    COM_CHECK(m_Utils->CreateDefaultIncludeHandler(&m_IncludeHandler));
}

ShaderDBImpl::~ShaderDBImpl() = default;

bool ShaderDBImpl::CompileShader(DRE::String64 const& name, DRE::ByteBuffer const& source, VKW::ShaderModuleType type)
{
    DXCArgsBuilder args;
    args.AddDebugArgs();
    args.AddEntryPoint("mainCS", VKW::SHADER_MODULE_TYPE_COMPUTE);

    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr = source.Data();
    sourceBuffer.Size = source.Size();
    sourceBuffer.Encoding = 0;

    Microsoft::WRL::ComPtr<IDxcResult> pCompileResult;

    m_Compiler->Compile(&sourceBuffer, args.m_CompilationArgs.Data(), args.m_CompilationArgs.Size(), m_IncludeHandler, IID_PPV_ARGS(&pCompileResult));

    Microsoft::WRL::ComPtr<IDxcBlobUtf8> pErrors;
    pCompileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);

    if (pErrors && pErrors->GetStringLength() > 0)
    {
        std::cerr << pErrors->GetStringPointer() << std::endl;
        DRE_ASSERT(false, "Shader failed to compile");
        return false;
    }
    else
    {
        Microsoft::WRL::ComPtr<IDxcBlob> outObject;
        pCompileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&outObject), nullptr);
        if (outObject->GetBufferSize() == 0)
        {
            DRE_ASSERT(false, "dxc failed to extract compilation result object");
            return false;
        }
        else
        {
            DRE::ByteBuffer spvBuffer{ outObject->GetBufferSize() };
            DRE::MemCpy(spvBuffer.Data(), outObject->GetBufferPointer(), spvBuffer.Size());

            m_ShaderMap.Emplace(name, ShaderEntry{
                .name = name,
                .type = type,
                .spirv = DRE_MOVE(spvBuffer),
                .source = source
                });

            return true;
        }

        /*ComPtr<IDxcBlob> pReflectionData;
        pCompileResult->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(pReflectionData.GetAddressOf()), nullptr);
        DxcBuffer reflectionBuffer;
        reflectionBuffer.Ptr = pReflectionData->GetBufferPointer();
        reflectionBuffer.Size = pReflectionData->GetBufferSize();
        reflectionBuffer.Encoding = 0;
        ComPtr<ID3D12ShaderReflection> pShaderReflection;
        pUtils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(pShaderReflection.GetAddressOf()));*/
    }
}

DRE::ByteBuffer const& ShaderDBImpl::GetShaderSpv(DRE::String64 const& name)
{
    auto result = m_ShaderMap.Find(name);
    DRE_ASSERT(result.value != nullptr, "Shader was not found in ShaderDB!");
    return result.value->spirv;
}
#undef COM_CHECK

}