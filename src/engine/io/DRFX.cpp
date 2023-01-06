#include <engine\io\DRFX.hpp>

/*

#include <fstream>
#include <string.h>

#include <foundation\Util\Hash.hpp>
#include <foundation\Container\InplaceHashTable.hpp>

namespace DRFX
{

char const* s_DICTIONARY[] =
{
    "Less"
    "Equals",
    "LessEquals",
    "Greater",
    "GreaterEquals",
    "Always",
    "Never",
    "Zero",
    "One",
    "Keep",
    "Replace",
    "Increment",
    "Decrement",
    "Vertex",
    "Fragment",
    "Compute",
    "Graphics",
    "Raytracing",
    "Front",
    "Back",
    "Solid",
    "Outline",
    "TriangleList",
    "None",
    "Alpha",
    "Texture",
    "Buffer",
    "Uniform"
};

DRE::InplaceHashTable<std::uint32_t, Keyword> m_HashKeywordMap;

void InitializeDictionary()
{
    for (std::uint16_t i = 0; i < (std::uint16_t)Keyword::MAX; i++)
    {
        std::uint32_t length = std::strlen(s_DICTIONARY[i]);
        m_HashKeywordMap[fasthash32(s_DICTIONARY[i], length, uint32_t(0xE527A10B))] = (Keyword)i;
    }
}

VertexState PushVertexState(char const* name)
{
    
}

DepthStencilState PushDepthStencilState(char const* name)
{

}

Document::Document(char const* name, char const* filePath)
    : Entity{ name }
{
    std::ifstream file{ filePath, std::ios_base::binary | std::ios_base::beg };
    if (file)
    {
        void* buffer = std::malloc(1024 * 64);
        void* writePtr = buffer;
        while (!file.eof())
        {
            //file.read(writePtr, 1024);
        }


        std::free(buffer);

    }
}

}
*/
