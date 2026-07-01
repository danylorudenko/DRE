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
    FFTButterfly,
    FFTH0,
    FFTHxt,
    FFTPingPong0,
    FFTPingPong1,
    WaterColor,
    WaterH0,
    WaterHeight,
    AmbientOcclusion,
    DisplayEncodedImage,
    ColorHistoryBuffer0,
    ColorHistoryBuffer1,
    GBufferA_DiffuseRoughness,
    GBufferB_NormalMetalness,
    GBufferC_Velocity,
    GBufferD_ObjectIDBuffer,
    DDGI_AtlasGBufferA,
    DDGI_AtlasGBufferB,
    DDGI_AtlasVisibility,
    DDGI_ProbeIrradiance,
    DEBUG_TEXTURE,
    ID_MAX
};

//////////////////////////////////////
enum class BufferID
{
    ID_None,
    BulletInstances,
    DDGI_ProbeData,
    DebugPassDDGIProbeIndirectArgs,
    DebugDrawBuffer,
    SortedDebugDrawBuffer,
    DebugDrawCounters,
    DebugDrawIndirectArgs,
    ID_MAX
};

#define RESOURCE_ID(id) #id

}

