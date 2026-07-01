#ifndef _DEBUG_DRAW_SLANG_
#define _DEBUG_DRAW_SLANG_

#include "common/shaders_defines.slang"

#define DEBUG_DRAW_COMMAND_CAPACITY 1024

enum DebugDrawCommandType : uint
{
    Line,           // p0->p1
    Sphere,         // p0.xyz=center, p0.w=radius
    AABB,           // p0.xyz=min, p1.xyz=max
    COUNT
};

struct S_DEBUG_DRAW_COMMAND
{
    DebugDrawCommandType type;
    uint flags;
    uint colorRGBA8;
    uint pad0;

    float4 p0;
    float4 p1;
};

struct DebugDrawBuffer
{
    uint commandCount;
    uint commandCapacity;
    uint pad0;
    uint pad1;

    S_DEBUG_DRAW_COMMAND commands[DEBUG_DRAW_COMMAND_CAPACITY];
};

struct DebugDrawCounters
{
    uint totalTypeCount[uint(DebugDrawCommandType::COUNT)];
    uint localSortingCounter[uint(DebugDrawCommandType::COUNT)];
};

#endif // _DEBUG_DRAW_SLANG_
