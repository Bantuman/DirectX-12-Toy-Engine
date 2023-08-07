#include "../CommonStructs.hlsli"

RWTexture2D<uint> visibilityBuffer : register(u0);
RWStructuredBuffer<uint2> offsetBuffer : register(u1);
RWStructuredBuffer<uint> materialCountBuffer : register(u2);
RWStructuredBuffer<uint> materialOffsetBuffer : register(u3);
RWStructuredBuffer<float2> pixelPositionBuffer : register(u4);

float4 main(FullscreenPixelInput input) : SV_TARGET
{
    float w;
    float h;
    visibilityBuffer.GetDimensions(w, h);

    uint packedvis = visibilityBuffer[uint2(w * input.TexCoord.x, h * input.TexCoord.y)].r;
    uint drawcallId = packedvis >> NUM_TRIANGLE_BITS;
    uint materialId = offsetBuffer[drawcallId].r;
    
    uint id = 0;
    InterlockedAdd(materialCountBuffer[materialId], 1, id);
    pixelPositionBuffer[materialOffsetBuffer[materialId] + id] = input.TexCoord;
    
    discard;
    return float4(0, 1, 0, 1);
}