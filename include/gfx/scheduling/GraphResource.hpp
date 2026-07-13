#pragma once


namespace GFX
{

enum GraphResourceFlags : DRE::U32
{
    NONE = 0,
    TEMPORAL = (1 << 0),
    INIT_CLEAR = (1 << 1),
};

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
    ColorHistoryBuffer,
    GBufferA_DiffuseRoughness,
    GBufferB_NormalMetalness,
    GBufferC_Velocity,
    GBufferD_ObjectIDBuffer,
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
    DDGI_ProbeSampleBuffer,
    DebugPassDDGIProbeIndirectArgs,
    DebugDrawBuffer,
    SortedDebugDrawBuffer,
    DebugDrawCounters,
    DebugDrawIndirectArgs,
    ID_MAX
};

#define RESOURCE_ID(id) #id

}

