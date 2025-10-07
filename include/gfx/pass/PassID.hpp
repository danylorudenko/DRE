#pragma once

namespace GFX
{

enum class PassID
{
    BulletForward,
    GBuffer,
    Lighting,
    ForwardOpaque,
    Shadow,
    Caustic,
    Water,
    FFTButterflyGen,
    FFTWaterH0Gen,
    FFTWaterHxtGen,
    FFTWaterHeightGen,
    FFTWaterInvPerm,
    AntiAliasing,
    AmbientOcclusion,
    ImGuiRender,
    ColorEncoding,
    Editor,
    Debug,
    MAX
};

}
