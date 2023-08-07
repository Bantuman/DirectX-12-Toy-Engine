#include "CommonStructs.hlsli"

RWTexture2D<uint> visibilityBuffer : register(u0);
RWStructuredBuffer<uint2> offsetBuffer : register(u1);
Texture2D<float> gBuffer : register(t0);

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
    uint drawCallId = packedvis.r >> NUM_TRIANGLE_BITS;
    uint triangleId = packedvis.r & TRIANGLE_MASK;
   // uint materialId = packedvis.g;
    
    float4 tri = GetProceduralColor(triangleId);
    float s = (tri.r + tri.g + tri.b) * (1 / 3.f);
    float4 clr = (s * 0.5f + 0.5f) * GetProceduralColor(drawCallId);

    float r = gBuffer[uint2(w * input.TexCoord.x, h * input.TexCoord.y)];
    float g = gBuffer[uint2(w * input.TexCoord.x + w, h * input.TexCoord.y)];
    float b = gBuffer[uint2(w * input.TexCoord.x + (2 * w), h * input.TexCoord.y)];
    float a = gBuffer[uint2(w * input.TexCoord.x + (3 * w), h * input.TexCoord.y)];
    
    return float4(r, g, b, a);
}