#pragma once

#include <foundation\Common.hpp>
#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>
#include <foundation\container\InplaceVector.hpp>
#include <foundation\container\InplaceHashTable.hpp>
#include <foundation\memory\ByteBuffer.hpp>
#include <foundation\memory\MemoryOps.hpp>
#include <foundation\string\InplaceString.hpp>

#include <engine\io\ShaderDB.hpp>

//#include <spirv_cross.hpp>
#include <wrl/client.h>
#include <dxcapi.h>

#include <slang.h>
#include <slang-com-ptr.h>

#include <thread>
#include <atomic>
#include <mutex>

#define DEBUG_SHADER_COMPILATION

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
    ShaderDBImpl(IO::IOManager* io);
    ~ShaderDBImpl();

    void                    CompileSources(bool parallel);

    bool                    CompileShader(DRE::String64 const& path, VKW::ShaderModuleType type);
    ShaderEntry const*      GetShaderEntry(DRE::String64 const& name);


    // shader recompilation
    void                                    ShaderObserver();

    void                                    ClearPendingShaders();
    inline bool                             AreNewShadersPending() const { return m_PendingChangesFlag.load(std::memory_order::acquire); }
    // move-returns pending shaders. Pending shaders are automatically "cleared" after this call
    DRE::InplaceVector<DRE::String64, 12>   GetPendingShaders();

private:
    IO::IOManager*                          m_IOManager;
    Slang::ComPtr<slang::IGlobalSession>    m_SlangGlobalSession;

    DRE::InplaceHashTable<DRE::String64, ShaderEntry, 512> m_ShaderMap;

private:
    DRE::InplaceVector<DRE::String64, 12> m_PendingShaders;
    std::mutex  m_PendingShadersMutex;
    std::thread m_ShaderObserverThread;
    std::atomic_bool m_PendingChangesFlag;

#ifdef DEBUG_SHADER_COMPILATION
    std::mutex  m_DebugShaderCompilationMutex;
#endif
};

}