#include "CommonStructs.hlsli"

struct CameraBuffer
{
    matrix View;
    matrix Projection;
};

ConstantBuffer<CameraBuffer> CameraCB : register(b0);
StructuredBuffer<float4x4> InstancesSB : register(t0, space0);

VertexShaderOutput main(VertexPositionNormalTangentBitangentTexture IN)
{
    VertexShaderOutput OUT;
    
    float4x4 world = InstancesSB[IN.InstanceId];
    float4x4 viewMatrix = mul(CameraCB.View, world);
    float4x4 viewProjection = mul(CameraCB.Projection, viewMatrix);
    float4x4 inverseViewMatrix = transpose(viewMatrix);
    
    OUT.PositionVS = mul(viewMatrix, float4(IN.Position, 1.0f));
    OUT.NormalVS = mul((float3x3) inverseViewMatrix, IN.Normal);
    OUT.TangentVS = mul((float3x3) inverseViewMatrix, IN.Tangent);
    OUT.BitangentVS = mul((float3x3) inverseViewMatrix, IN.Bitangent);
    OUT.TexCoord = IN.TexCoord.xy;
    OUT.Position = mul(viewProjection, float4(IN.Position, 1.0f));
    return OUT;
}

//struct Matrices
//{
//    matrix ModelMatrix;
//    matrix ModelViewMatrix;
//    matrix InverseTransposeModelViewMatrix;
//    matrix ModelViewProjectionMatrix;
//};

//ConstantBuffer<Matrices> MatCB : register(b0);

//VertexShaderOutput main(VertexPositionNormalTangentBitangentTexture IN)
//{
//    VertexShaderOutput OUT;
//    OUT.PositionVS = mul(MatCB.ModelViewMatrix, float4(IN.Position, 1.0f));
//    OUT.NormalVS = mul((float3x3) MatCB.InverseTransposeModelViewMatrix, IN.Normal);
//    OUT.TangentVS = mul((float3x3) MatCB.InverseTransposeModelViewMatrix, IN.Tangent);
//    OUT.BitangentVS = mul((float3x3) MatCB.InverseTransposeModelViewMatrix, IN.Bitangent);
//    OUT.TexCoord = IN.TexCoord.xy;
//    OUT.Position = mul(MatCB.ModelViewProjectionMatrix, float4(IN.Position, 1.0f));
//    return OUT;
//}