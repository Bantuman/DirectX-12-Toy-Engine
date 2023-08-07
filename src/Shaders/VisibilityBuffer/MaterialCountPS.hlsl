#include "../CommonStructs.hlsli"

RWTexture2D<uint> visibilityBuffer : register(u0);
RWStructuredBuffer<uint> materialCountBuffer : register(u1);

float4 GetProceduralColor(int index)
{
    // Generate the color based on the index
    float4 color = float4(
        frac(sin(index * 532.23) * 103.74),
        frac(sin(index * 743.28) * 459.98),
        frac(sin(index * 233.92) * 832.17),
        1.0
    );

    return color;
}

float4 main(FullscreenPixelInput input) : SV_TARGET
{
    float w;
    float h;
    visibilityBuffer.GetDimensions(w, h);

    uint packedvis = visibilityBuffer[uint2(w * input.TexCoord.x, h * input.TexCoord.y)].r;
    uint materialId = packedvis >> NUM_TRIANGLE_BITS;
    
    uint count;
    InterlockedAdd(materialCountBuffer[materialId], 1, count);
   
   // materialCountBuffer.InterlockedAdd(materialId, 1, count);

    discard;
    return float4(1, 0, 0, 1);
}