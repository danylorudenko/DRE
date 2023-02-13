#pragma once

#include <foundation/Common.hpp>
#include <string.h>

DRE_BEGIN_NAMESPACE

template<typename TChar, U16 SIZE>
class InplaceString
{
public:
    InplaceString()
        : m_Size{ 0 }
    {
        std::memset(m_Data, 0, sizeof(m_Data));
    }

    template<typename TString>
    InplaceString(TString&& rhs)
        : m_Size{ static_cast<DRE::U16>(std::strlen(rhs)) }
    {
        DRE_ASSERT(m_Size < SIZE, "Inplace string overflow.");
        std::memset(m_Data, 0, sizeof(m_Data));
        std::strcpy(m_Data, rhs);
    }

    template<typename TString>
    bool operator==(TString&& rhs)
    {
        return std::strcmp(m_Data, rhs) == 0;
    }

    template<typename TString>
    bool operator!=(TString&& rhs)
    {
        return std::strcmp(m_Data, rhs) != 0;
    }

    operator TChar* ()
    {
        return m_Data;
    }

    operator TChar const* () const
    {
        return m_Data;
    }

    U16 GetSize() const { return m_Size; }

    void Shrink(U16 size)
    {
        DRE_ASSERT(size <= m_Size, "Can't shrink string upwards");
        m_Data[size - 1] = '\0';
        m_Size = size;
    }

    void Append(char const* str)
    {
        auto length = std::strlen(str);
        Append(str, length);
    }

    void Append(char const* str, U16 size)
    {
        DRE_ASSERT(size + m_Size < SIZE, "Inplace string overflow.");
        std::strncat(m_Data, str, size);
        m_Size += size;
        m_Data[m_Size] = '\0';
    }

private:
    U16     m_Size;
    TChar   m_Data[SIZE];
};


using String32      = InplaceString<char, 32>;
using String64      = InplaceString<char, 64>;
using String128     = InplaceString<char, 128>;
using String256     = InplaceString<char, 256>;
using String512     = InplaceString<char, 512>;

DRE_END_NAMESPACE
