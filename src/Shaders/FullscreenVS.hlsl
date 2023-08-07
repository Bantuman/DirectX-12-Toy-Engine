#include "CommonStructs.hlsli"

FullscreenPixelInput main(uint vertexid : SV_VertexID)
{
    FullscreenPixelInput result;
    result.TexCoord = float2((vertexid << 1) & 2, vertexid & 2);
    result.Position = float4(result.TexCoord * float2(2, -2) + float2(-1, 1), 0, 1);
    return result;
}