#include "../CommonStructs.hlsli"

struct PixelShaderInput
{
    float4 PositionVS : POSITION;
    float3 NormalVS : NORMAL;
    float3 TangentVS : TANGENT;
    float3 BitangentVS : BITANGENT;
    float2 TexCoord : TEXCOORD;
    
    uint TriangleID : SV_PrimitiveID;
};

SamplerState linearSampler : register(s0);
RWStructuredBuffer<uint2> offsetBuffer : register(u0);

cbuffer DrawCallInfo : register(b0, space1)
{
    uint DrawCallId;
    uint MaterialId;
    uint MeshVertexOffset;
    uint MeshIndexOffset;
}

uint main(PixelShaderInput IN) : SV_Target
{
    //return uint2((MaterialId << NUM_TRIANGLE_BITS) | (MeshIndexOffset + (IN.TriangleID * 3)), | MeshVertexOffset);
    offsetBuffer[DrawCallId] = uint2(MaterialId, MeshVertexOffset);
    return (DrawCallId << NUM_TRIANGLE_BITS) | (MeshIndexOffset + (IN.TriangleID * 3));
}