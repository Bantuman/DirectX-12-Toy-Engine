#include "../CommonStructs.hlsli"

RWTexture2D<uint> visibilityBuffer : register(u0);
RWStructuredBuffer<uint2> offsetBuffer : register(u1);
RWStructuredBuffer<uint> materialCountBuffer : register(u2);

#define GROUP_SIZE 32

groupshared uint intermediateCountBuffer[GROUP_SIZE * GROUP_SIZE];
groupshared uint intermediateCounterBuffer[MATERIAL_UPPER_LIMIT];

[numthreads(GROUP_SIZE, GROUP_SIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint GTid : SV_GroupIndex)
{
    float w;
    float h;
    visibilityBuffer.GetDimensions(w, h);
    
    if (DTid.x >= w || DTid.y >= h)
    {
        intermediateCountBuffer[GTid] = ~0;
    } 
    else
    {
        uint packedvis = visibilityBuffer[DTid.xy].r;
        uint drawCallId = packedvis.r >> NUM_TRIANGLE_BITS;
        uint materialId = offsetBuffer[drawCallId].r;
        
        intermediateCountBuffer[GTid] = materialId;
    }
    
    if (GTid < MATERIAL_UPPER_LIMIT)
    {
        intermediateCounterBuffer[GTid] = 0;
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    const uint id = intermediateCountBuffer[GTid];
    if (id < MATERIAL_UPPER_LIMIT)
    {
        InterlockedAdd(intermediateCounterBuffer[id], 1);
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    if (GTid < MATERIAL_UPPER_LIMIT)
    {
        if (intermediateCounterBuffer[GTid] > 0)
        {
            InterlockedAdd(materialCountBuffer[GTid], intermediateCounterBuffer[GTid]);
        }
    }
}


