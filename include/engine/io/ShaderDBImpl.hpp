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

    bool                    CompileShaderFile(DRE::String128 const& path);

    ShaderFile const*       GetShaderFile(DRE::String128 const& name);
    ShaderEntry const*      GetShaderEntry(DRE::String128 const& name);

    static DRE::String128   BuildShaderPath(char const* shaderFile);


    // shader recompilation
    void                                    ShaderObserver_Thread();

    void                                    ClearPendingShaders();
    inline bool                             AreNewShadersPending() const { return m_PendingChangesFlag.load(std::memory_order::acquire); }
    // move-returns pending shaders. Pending shaders are automatically "cleared" after this call
    DRE::InplaceVector<DRE::String128, 12>  ReadAndClearPendingShaderFilesCopy();

    DRE::InplaceHashTable<DRE::String128, DRE::String128, 512> const& GetShaderToFileMap() const { return m_ShaderToFileMap; }

private:
    IO::IOManager*                          m_IOManager;
    Slang::ComPtr<slang::IGlobalSession>    m_SlangGlobalSession;

    DRE::InplaceHashTable<DRE::String128, ShaderFile, 512>       m_ShaderFileMap;
    DRE::InplaceHashTable<DRE::String128, ShaderEntry, 512>      m_ShaderEntryMap;
    DRE::InplaceHashTable<DRE::String128, DRE::String128, 512>   m_ShaderToFileMap;

private:
    DRE::InplaceVector<DRE::String128, 12> m_PendingShaderFiles;
    std::mutex  m_PendingShaderFilesMutex;
    std::thread m_ShaderObserverThread;
    std::atomic_bool m_PendingChangesFlag;

#ifdef DEBUG_SHADER_COMPILATION
    std::mutex  m_DebugShaderCompilationMutex;
#endif
};

}