#pragma once


namespace GFX
{

//////////////////////////////////////
enum class TextureID
{
    ID_None,
    MainDepth,
    ShadowMap,
    CausticEnvMap,
    CausticMap,
    ForwardColor,
    WaterColor,
    Velocity,
    DisplayEncodedImage,
    ColorHistoryBuffer0,
    ColorHistoryBuffer1,
    ID_MAX
};

//////////////////////////////////////
enum class BufferID
{
    ID_None,
    BulletInstances,
    ID_MAX
};


}

