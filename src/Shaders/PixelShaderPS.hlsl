#include "CommonStructs.hlsli"

struct PixelShaderInput
{
    float4 PositionVS : POSITION;
    float3 NormalVS : NORMAL;
    float3 TangentVS : TANGENT;
    float3 BitangentVS : BITANGENT;
    float2 TexCoord : TEXCOORD;
};

SamplerState linearSampler : register(s0);

Texture2D tex : register(t2);

float4 main(PixelShaderInput IN) : SV_Target
{
    float4 color = tex.Sample(linearSampler, IN.TexCoord);
    return color;
}