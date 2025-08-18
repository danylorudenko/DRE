#pragma once

#include <foundation\Common.hpp>
#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>
#include <foundation\container\InplaceVector.hpp>
#include <foundation\container\InplaceHashTable.hpp>
#include <foundation\memory\ByteBuffer.hpp>
#include <foundation\memory\MemoryOps.hpp>
#include <foundation\string\InplaceString.hpp>

#include <vk_wrapper\pipeline\ShaderModule.hpp>

#include <spirv_cross.hpp>
#include <wrl/client.h>
#include <dxcapi.h>

#include <slang.h>
#include <slang-com-ptr.h>

namespace IO
{

class IOManager;

// keep this object until hlsl was compiled
/////////////////////////////////
// DXCArgsBuilder
struct DXCArgsBuilder
{
    DXCArgsBuilder();

    void AddDebugArgs();
    void AddOptimizedArgs();
    void AddEntryPoint(char const* entryPoint, VKW::ShaderModuleType type);
    void AppendGenericArgument(char const* input);
    void AddGenericArgument(char const* prefix, char const* argument);

private:
    void PeekGenericBufferSize(DRE::U32 appendSize);

    void AddProfileArgs(VKW::ShaderModuleType type);

    static DRE::U32 constexpr GENERIC_ARGUMENT_BUFFER_SIZE = 512;
    wchar_t m_GenericArgsBuffer[GENERIC_ARGUMENT_BUFFER_SIZE];
    DRE::U32 m_GenericArgsBufferOffset = 0;

public:
    DRE::InplaceVector<LPCWSTR, 32> m_CompilationArgs;
};


/////////////////////////////////
// ShaderModuleDB
class ShaderDBImpl
    : public NonMovable
    , public NonCopyable
{
public:
    struct ShaderEntry
    {
        DRE::String64           name;
        VKW::ShaderModuleType   type;
        DRE::ByteBuffer         spirv;
        DRE::ByteBuffer         source;
    };

    ShaderDBImpl(IO::IOManager* io);
    ~ShaderDBImpl();

    bool                    CompileShader(DRE::String64 const& name, DRE::ByteBuffer const& source, VKW::ShaderModuleType type);
    DRE::ByteBuffer const&  GetShaderSpv(DRE::String64 const& name);

private:
    IO::IOManager*                          m_IOManager;

    Microsoft::WRL::ComPtr<IDxcUtils>       m_Utils;
    Microsoft::WRL::ComPtr<IDxcCompiler3>   m_Compiler;
    IDxcIncludeHandler*                     m_IncludeHandler;



    Slang::ComPtr<slang::IGlobalSession>    m_SlangGlobalSession;



    DRE::InplaceVector<LPCWSTR, 64>         m_DefaultCompilationArgs;

    DRE::InplaceHashTable<DRE::String64, ShaderEntry, 512> m_ShaderMap;

#undef COM_CHECK
};

}