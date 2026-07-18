#include "common/shaders_common.slang"
#include "debug_view.slang"
#include "common/global_illumination/ddgi_common.slang"

float4 RemapDisplayValue(float4 value, float lowBound, float highBound)
{
    float distance = highBound - lowBound;
    return (value - lowBound) / distance;
}

[numthreads(8, 8, 1)]
void mainCS(uint3 DTid : SV_DispatchThreadID)
{
    //if (IsTextureDebugView(debugViewArgs))
    {
        int2 coords = int2(DTid.xy);
        float2 uv = float2(coords - 50) / GetViewportSize();
        uv += float2(debugViewArgs.offsetX, debugViewArgs.offsetY);
        uv /= float2(debugViewArgs.sizeX, debugViewArgs.sizeY);

        if (all(uv <= 1.0) && all(uv >= 0.0))
        {
            float4 textureValue = SampleGlobalTextureLinear(debugViewArgs.textureID, uv, 0.0);

            uint4 channelMask = uint4(
                GetChannelX(debugViewArgs),
                GetChannelY(debugViewArgs),
                GetChannelZ(debugViewArgs),
                GetChannelW(debugViewArgs)
            );

            float4 channelMaskFloat = float4(channelMask);
            float4 displayValue = textureValue * channelMaskFloat;

            // special case when single channel is selected
            if (channelMask.x + channelMask.y + channelMask.z + channelMask.w == 1)
            {
                displayValue = (displayValue.x + displayValue.y + displayValue.z + displayValue.w).xxxx;
            }

            displayValue = RemapDisplayValue(displayValue, debugViewArgs.lowBound, debugViewArgs.highBound);
            outputTexture[coords] = displayValue;
        }
    }
}