#ifndef _DEBUG_DRAW_SLANG_
#define _DEBUG_DRAW_SLANG_

#include "common/shaders_defines.slang"

#define DEBUG_DRAW_COMMAND_CAPACITY 1024

enum DebugDrawCommandType : uint
{
    Line        = 0, // p0->p1
    Sphere      = 1, // p0.xyz=center, p0.w=radius
    AABB        = 2, // p0.xyz=min, p1.xyz=max
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
    uint debugDrawEnabled;
    uint commandCount;
    uint commandCapacity;
    uint pad0;

    S_DEBUG_DRAW_COMMAND commands[DEBUG_DRAW_COMMAND_CAPACITY];
};

#endif // _DEBUG_DRAW_SLANG_
