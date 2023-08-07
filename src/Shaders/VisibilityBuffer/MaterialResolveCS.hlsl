#include "../CommonStructs.hlsli"

RWTexture2D<uint> visibilityBuffer : register(u0);
RWStructuredBuffer<uint2> offsetBuffer : register(u1);
RWStructuredBuffer<uint> materialCountBuffer : register(u2);
RWStructuredBuffer<uint> materialOffsetBuffer : register(u3);
RWStructuredBuffer<float2> pixelPositionBuffer : register(u4);
RWTexture2D<float> renderTarget: register(u5);

struct VertexData
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float3 Bitangent : BITANGENT;
    float3 TexCoord : TEXCOORD;
};

RWStructuredBuffer<VertexData> visibleVertexBuffer : register(u7);
RWStructuredBuffer<uint> visibleIndexBuffer : register(u8);

Texture2D<float4> albedoTexture[MATERIAL_UPPER_LIMIT] : register(t1);

SamplerState linearSampler : register(s0);

cbuffer MaterialOffsetBuffer : register(b0, space1)
{
    uint offset;
    uint count;
}

struct CameraBuffer
{
    matrix View;
    matrix Projection;
    matrix InverseViewProjection;
};

ConstantBuffer<CameraBuffer> CameraCB : register(b1);
StructuredBuffer<float4x4> InstancesSB : register(t0, space0);

float4 GetProceduralColor(int index)
{
    float4 color = float4(
        frac(sin(index * 532.23) * 103.74),
        frac(sin(index * 743.28) * 459.98),
        frac(sin(index * 233.92) * 832.17),
        1.0
    );

    return color;
}

struct BarycentricDeriv
{
    float3 m_lambda;
    float3 m_ddx;
    float3 m_ddy;
};

struct TrianglePos
{
    float3 tri0;
    float3 tri1;
    float3 tri2;
};

BarycentricDeriv CalcFullBary(float4 pt0, float4 pt1, float4 pt2, float2 pixelNdc, float2 winSize)
{
    BarycentricDeriv ret;
    float3 invW = 1.0f / float3(pt0.w, pt1.w, pt2.w);
    float2 ndc0 = pt0.xy * invW.x;
    float2 ndc1 = pt1.xy * invW.y;
    float2 ndc2 = pt2.xy * invW.z;

    float invDet = rcp(determinant(float2x2(ndc2 - ndc1, ndc0 - ndc1)));

    ret.m_ddx = float3(ndc1.y - ndc2.y, ndc2.y - ndc0.y, ndc0.y - ndc1.y) * invDet * invW;
    ret.m_ddy = float3(ndc2.x - ndc1.x, ndc0.x - ndc2.x, ndc1.x - ndc0.x) * invDet * invW;
    float ddxSum = dot(ret.m_ddx, float3(1, 1, 1));
    float ddySum = dot(ret.m_ddy, float3(1, 1, 1));
	
    float2 deltaVec = pixelNdc - ndc0;

    float interpInvW = invW.x + deltaVec.x * ddxSum + deltaVec.y * ddySum;
    float interpW = rcp(interpInvW);
    ret.m_lambda.x = interpW * (invW[0] + deltaVec.x * ret.m_ddx.x + deltaVec.y * ret.m_ddy.x);
    ret.m_lambda.y = interpW * (0.0f + deltaVec.x * ret.m_ddx.y + deltaVec.y * ret.m_ddy.y);
    ret.m_lambda.z = interpW * (0.0f + deltaVec.x * ret.m_ddx.z + deltaVec.y * ret.m_ddy.z);

    ret.m_ddx *= (2.0f / winSize.x);
    ret.m_ddy *= (2.0f / winSize.y);
    ddxSum *= (2.0f / winSize.x);
    ddySum *= (2.0f / winSize.y);

    float interpW_ddx = 1.0f / (interpInvW + ddxSum);
    float interpW_ddy = 1.0f / (interpInvW + ddySum);

    ret.m_ddx = interpW_ddx * (ret.m_lambda * interpInvW + ret.m_ddx) - ret.m_lambda;
    ret.m_ddy = interpW_ddy * (ret.m_lambda * interpInvW + ret.m_ddy) - ret.m_lambda;

    return ret;
}

float3 InterpolateWithDeriv3(BarycentricDeriv deriv, float v0, float v1, float v2)
{
    float3 mergedV = float3(v0, v1, v2);
    float3 ret;
    ret.x = dot(mergedV, deriv.m_lambda);
    ret.y = dot(mergedV, deriv.m_ddx);
    ret.z = dot(mergedV, deriv.m_ddy);
    return ret;
}

float InterpolateWithDeriv(BarycentricDeriv deriv, float3 v)
{
    return dot(v, deriv.m_lambda);
}
float InterpolateWithDeriv(BarycentricDeriv deriv, float v0, float v1, float v2)
{
    float3 mergedV = float3(v0, v1, v2);
    return InterpolateWithDeriv(deriv, mergedV);
}

float3 InterpolateWithDeriv(BarycentricDeriv deriv, float3x3 attributes)
{
    //float3 attr0 = attributes._11_21_31;
    //float3 attr1 = attributes._12_22_32;
    //float3 attr2 = attributes._13_23_33;
    float3 attr0 = attributes._11_12_13;
    float3 attr1 = attributes._21_22_23;
    float3 attr2 = attributes._31_32_33;
    return float3(dot(attr0, deriv.m_lambda), dot(attr1, deriv.m_lambda), dot(attr2, deriv.m_lambda));
}

struct GradientInterpolationResults
{
    float2 interp;
    float2 dx;
    float2 dy;
};

GradientInterpolationResults Interpolate2DWithDeriv(BarycentricDeriv deriv, float3x2 attributes)
{
    float3 attr0 = attributes._11_21_31;
    float3 attr1 = attributes._12_22_32;
	
    GradientInterpolationResults result;
    result.interp.x = InterpolateWithDeriv(deriv, attr0);
    result.interp.y = InterpolateWithDeriv(deriv, attr1);

    result.dx.x = dot(attr0, deriv.m_ddx);
    result.dx.y = dot(attr1, deriv.m_ddx);
    result.dy.x = dot(attr0, deriv.m_ddy);
    result.dy.y = dot(attr1, deriv.m_ddy);
    return result;
}

uint3 FetchTriangleIndices(uint primId)
{
    uint3 indices;
    indices.x = visibleIndexBuffer[primId];
    indices.y = visibleIndexBuffer[primId + 1];
    indices.z = visibleIndexBuffer[primId + 2];
    return indices;
}

struct TriangleData
{
    VertexData triangle0;
    VertexData triangle1;
    VertexData triangle2;
};

TriangleData FetchTriangle(uint vertexOffset, uint3 indices)
{
    TriangleData data;
    data.triangle0 = visibleVertexBuffer[vertexOffset + indices.x];
    data.triangle1 = visibleVertexBuffer[vertexOffset + indices.y];
    data.triangle2 = visibleVertexBuffer[vertexOffset + indices.z];
    return data;
}

float depthLinearization(float depth, float near, float far)
{
    return (2.0 * near) / (far + near - depth * (far - near));
}

[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID)
{
    uint id = DTid.x;
    
    if (id > count)
    {
        return;
    }
    if (id < MATERIAL_UPPER_LIMIT)
    {
        materialOffsetBuffer[id] = 0;
        materialCountBuffer[id] = 0;
    }
    
    uint width;
    uint height;
    visibilityBuffer.GetDimensions(width, height);
    
    float2 position = pixelPositionBuffer[offset + id];
    uint2 coords = uint2(position.x * width, position.y * height);
    
    if (coords.x > width || coords.y > height)
    {
        return;
    }
    
    uint packedvis = visibilityBuffer[coords];
    uint drawCallId = packedvis >> NUM_TRIANGLE_BITS;
    uint triangleId = packedvis & TRIANGLE_MASK;
    
    uint2 offsetData = offsetBuffer[drawCallId].rg;
    uint materialId = offsetData.r;
    uint vertexOffset = offsetData.g;
   
   // float4 tri = GetProceduralColor(vertexOffset);
    
    uint3 indices = FetchTriangleIndices(triangleId);
    TriangleData tri = FetchTriangle(vertexOffset, indices);
    
    // TODO: Replace [0] with instance index
    float4x4 world = InstancesSB[0];
    float4x4 viewMatrix = mul(CameraCB.View, world);
    float4x4 viewProjection = mul(CameraCB.Projection, viewMatrix);
   // float4x4 inverseViewMatrix = transpose(viewMatrix);
    float4x4 inverseViewProjection = CameraCB.InverseViewProjection;
   
    float4 vmp0 = mul(viewProjection, float4(tri.triangle0.Position, 1.0f));
    float4 vmp1 = mul(viewProjection, float4(tri.triangle1.Position, 1.0f));
    float4 vmp2 = mul(viewProjection, float4(tri.triangle2.Position, 1.0f));
    
    //float4 wpos0 = mul(inverseViewProjection, vmp0);
    //float4 wpos1 = mul(inverseViewProjection, vmp1);
    //float4 wpos2 = mul(inverseViewProjection, vmp2);
    
    float3 one_over_w = 1.0f / float3(vmp0.w, vmp1.w, vmp2.w);
    
    vmp0 *= one_over_w.x;
    vmp1 *= one_over_w.y;
    vmp2 *= one_over_w.z;
    
    float2 screenpos = position * 2 - 1;
    screenpos.y *= -1;
    
    BarycentricDeriv derivatives = CalcFullBary(vmp0, vmp1, vmp2, screenpos, float2(width, height));
    float w = 1.0f / dot(one_over_w, derivatives.m_lambda);
    
    float z = w * CameraCB.Projection[2][2] + CameraCB.Projection[2][3];
    
   // float3 worldPosition = mul(inverseViewProjection, float4(screenpos * w, z, w)).xyz;
    
    float3x2 texCoords = float3x2(
			tri.triangle0.TexCoord.xy * one_over_w.x,
			tri.triangle1.TexCoord.xy * one_over_w.y,
			tri.triangle2.TexCoord.xy * one_over_w.z
	);
    
    GradientInterpolationResults results = Interpolate2DWithDeriv(derivatives, texCoords);
			
    // TODO: Break this out to a cbuffer again
    float nearPlane = 1;
    float farPlane = 5000;
    
    float linearZ = depthLinearization(z / w, nearPlane, farPlane);
    float mip = pow(pow(linearZ, 0.9f) * 5.0f, 1.5f);
	
    float2 texCoordDX = results.dx * mip;
    float2 texCoordDY = results.dy * mip;
    float2 texCoord = results.interp;
    
    texCoord *= w;
    texCoordDX *= w;
    texCoordDY *= w;
    
 //   float3x3 tangents = float3x3(
	//		tri.triangle0.Tangent * one_over_w[0],
 //           tri.triangle0.Tangent * one_over_w[1],
 //           tri.triangle0.Tangent * one_over_w[2]
	//);
    
 //   float3 tangent = normalize(InterpolateWithDeriv(derivatives, tangents));
    
   // renderTarget[coords] = float4(texCoord, 0, 1);
    //float3 avg = (positions.tri0 + positions.tri1 + positions.tri2) / 3;
   // float3 avg = (vmp0 + vmp1 + vmp2) / 3;
   // renderTarget[coords] = float4(texCoordDX.xy, 1, 1);
    float4 finalColor = albedoTexture[materialId].SampleGrad(linearSampler, texCoord, texCoordDX, texCoordDY);
    
    renderTarget[coords] = finalColor.r;
    renderTarget[coords + uint2(width, 0)] = finalColor.g;
    renderTarget[coords + uint2(2 * width, 0)] = finalColor.b;
    renderTarget[coords + uint2(3 * width, 0)] = finalColor.a;
    
  //  uint2 coords = uint2(id % width, id / width);
  //  uint2 packedvis = visibilityBuffer[coords].rg;
  //  uint drawCallId = packedvis.r >> NUM_TRIANGLE_BITS;
  //  uint triangleId = packedvis.r & TRIANGLE_MASK;
  //  uint materialId = packedvis.g;
    
    
  ////  uint count = materialCountBuffer[materialId]
  //  renderTarget[coords] = float4(1, 0, 0, 1);
}