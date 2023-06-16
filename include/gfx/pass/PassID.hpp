#pragma once

namespace GFX
{

enum class PassID
{
    BulletForward,
    ForwardOpaque,
    Shadow,
    Caustic,
    Water,
    FFTButterflyGen,
    FFTWaterH0Gen,
    FFTWaterHxtGen,
    FFTWaterHeightGen,
    FFTWater,
    AntiAliasing,
    ImGuiRender,
    ColorEncoding,
    Debug,
    MAX
};

}
