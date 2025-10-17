#pragma once

#include <foundation/Common.hpp>
#include <string.h>

DRE_BEGIN_NAMESPACE

template<typename TUniqueContext>
void GenerateUniqueLabel(char* dest, DRE::U32 destSize, char const* prefix, TUniqueContext const& context)
{
    char idBuffer[16];
    std::sprintf(idBuffer, "%u", context.GetUniqueID());

    DRE::SizeT prefixSize = std::strlen(prefix);
    DRE_ASSERT(destSize <= prefixSize + sizeof(idBuffer), "GenerateUniqueLabel couldn't fit destination buffer.");

    std::sprintf(dest, "%s%s", prefix, idBuffer);
}

DRE_END_NAMESPACE
