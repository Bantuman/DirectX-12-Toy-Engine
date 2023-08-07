#pragma once

#define MATERIAL_UPPER_LIMIT 32
#define NUM_TRIANGLE_BITS 23
#define TRIANGLE_MASK 0x7fffff
//0x7fffff 
//(0xFFFFFF) >> 1

struct VertexPositionNormalTangentBitangentTexture
{
    uint InstanceId : SV_InstanceID;
    
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float3 Bitangent : BITANGENT;
    float3 TexCoord : TEXCOORD;
};

struct VertexShaderOutput
{
    float4 PositionVS : POSITION;
    float3 NormalVS : NORMAL;
    float3 TangentVS : TANGENT;
    float3 BitangentVS : BITANGENT;
    float2 TexCoord : TEXCOORD;
    float4 Position : SV_Position;
};

struct FullscreenPixelInput
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD;
};