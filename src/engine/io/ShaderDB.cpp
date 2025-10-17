#pragma once

#include <engine\io\ShaderDB.hpp>
#include <engine\io\ShaderDBImpl.hpp>

#include <foundation\memory\Memory.hpp>

namespace IO
{

ShaderDB::ShaderDB(IO::IOManager* io)
    : m_Impl{ nullptr }
{
    m_Impl = DRE::g_PersistentDataAllocator.Alloc<ShaderDBImpl>(io);
}

ShaderDB::~ShaderDB()
{
    DRE::g_PersistentDataAllocator.FreeObject(m_Impl);
}

void ShaderDB::CompileSources(bool parallel)
{
    m_Impl->CompileSources(parallel);
}

bool ShaderDB::CompileShader(DRE::String64 const& name, VKW::ShaderModuleType type)
{
   return m_Impl->CompileShader(name, type);
}

ShaderEntry const* ShaderDB::GetShaderEntry(DRE::String64 const& name)
{
    return m_Impl->GetShaderEntry(name);
}

bool ShaderDB::NewShadersPending() const
{
    return m_Impl->NewShadersPending();
}

DRE::InplaceVector<DRE::String64, 12> ShaderDB::GetPendingShaders()
{
    return m_Impl->GetPendingShaders();
}

}