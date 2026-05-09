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

bool ShaderDB::CompileShaderFile(DRE::String64 const& name)
{
   return m_Impl->CompileShaderFile(name);
}

ShaderFile const* ShaderDB::GetShaderFile(DRE::String128 const& name)
{
    return m_Impl->GetShaderFile(name);
}

ShaderEntry const* ShaderDB::GetShaderEntry(DRE::String128 const& name)
{
    return m_Impl->GetShaderEntry(name);
}

DRE::String128 ShaderDB::BuildShaderPath(char const* shaderFile)
{
    return ShaderDBImpl::BuildShaderPath(shaderFile);
}

bool ShaderDB::AreNewShadersPending() const
{
    return m_Impl->AreNewShadersPending();
}

void ShaderDB::ClearPendingShaders()
{
    return m_Impl->ClearPendingShaders();
}

DRE::InplaceVector<DRE::String128, 12> ShaderDB::GetPendingShaderFilesCopy()
{
    return m_Impl->GetPendingShaderFilesCopy();
}

DRE::InplaceHashTable<DRE::String128, DRE::String128, 512> const& ShaderDB::GetShaderToFileMap() const
{
    return m_Impl->GetShaderToFileMap();
}

}