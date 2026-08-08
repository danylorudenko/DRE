#include "common/shaders_common.slang"
#include "common/global_illumination/ddgi_common.slang"

[[vk::binding(0, 3)]] ConstantBuffer<DDGIConstantBuffer>    ddgiCB;
[[vk::binding(1, 3)]] RWTexture2D                           ddgiAtlas;

void FillFullRow(uint rowLength, uint4 probeRect, bool topRow, bool debugFill)
{
    uint sourceRowOffset = topRow ? 1 : -1;
    uint Row = topRow ? probeRect.y : probeRect.w - 1;
    uint OppositeInternalRow = topRow ? probeRect.w - 2 : probeRect.y + 1;

    for(uint i = 0; i < rowLength; i++)
    {
        uint2 pixel = uint2(probeRect.x + i, Row);
        if (debugFill)
        {
            ddgiAtlas[pixel] = float4(1, 0, 1, 1);
            continue;
        }

        // left corner pixel
        if (i == 0)
        {
            uint2 sourcePixel = uint2(probeRect.z - 2, OppositeInternalRow);
            ddgiAtlas[pixel] = ddgiAtlas[sourcePixel];
        }
        // right corner pixel
        else if (i == rowLength - 1)
        {
            uint2 sourcePixel = uint2(probeRect.x + 1, OppositeInternalRow);
            ddgiAtlas[pixel] = ddgiAtlas[sourcePixel];
        }
        else
        {
            uint rowMirrorI = (probeRect.z - 1) - i;
            uint2 sourcePixel = uint2(rowMirrorI, Row + sourceRowOffset);
            ddgiAtlas[pixel] = ddgiAtlas[sourcePixel];
        }
    }
}



[shader("compute")]
[numthreads(4, 4, 4)]
void mainCS(uint3 DTid : SV_DispatchThreadID)
{
    if (any(DTid >= ddgiCB.probeGridDimentions))
        return;

    bool isVisibilityPass = (globalPushConstant.value_int1 & DDGI_VISIBILITY_FILL_PASS_BIT) != 0;

    uint2 probeResolution = isVisibilityPass ? ddgiCB.probeVisibilityResolution : ddgiCB.probeIrradianceResolution;
    uint probeIndex = GetProbeIndex(ddgiCB.probeGridDimentions, DTid);
    uint4 probeRect = GetDDGIProbeRect(probeIndex, probeResolution, ddgiCB.probeGridDimentions);

    bool debugFill = (ddgiCB.debugHighlightProbeTile != 0) && all(GetProbe3DIndex(ddgiCB.probeGridDimentions, probeIndex) == ddgiCB.debugProbeFocus);

    FillFullRow(probeResolution.x, probeRect, /*topRow*/true, /*debugFill*/debugFill);

    for(uint i = 1; i < probeResolution.y - 1; i++) // -1 to skip last row, which is filled later by FillFullRow
    {
        uint2 pixel0 = uint2(probeRect.x, probeRect.y + i);
        uint2 pixel1 = uint2(probeRect.z - 1, probeRect.y + i);

        uint2 sourcePixel0 = uint2(pixel0.x + 1, (probeRect.w - 1) - i);
        uint2 sourcePixel1 = uint2(pixel1.x - 1, (probeRect.w - 1) - i);

        if (!debugFill)
        {
            ddgiAtlas[pixel0] = ddgiAtlas[sourcePixel0];
            ddgiAtlas[pixel1] = ddgiAtlas[sourcePixel1];
        }
        else
        {
            ddgiAtlas[pixel0] = float4(1, 0, 1, 1);
            ddgiAtlas[pixel1] = float4(1, 0, 1, 1);
        }
    }

    FillFullRow(probeResolution.x, probeRect, /*topRow*/false, /*debugFill*/debugFill);

}