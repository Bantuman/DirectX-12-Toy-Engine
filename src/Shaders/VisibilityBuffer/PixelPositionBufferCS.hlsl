#include "../CommonStructs.hlsli"

RWTexture2D<uint> visibilityBuffer : register(u0);
RWStructuredBuffer<uint2> offsetBuffer : register(u1);
RWStructuredBuffer<uint> materialCountBuffer : register(u2);
RWStructuredBuffer<uint> materialOffsetBuffer : register(u3);
RWStructuredBuffer<float2> pixelPositionBuffer : register(u4);

#define GROUP_SIZE_X 4
#define GROUP_SIZE_Y 8
#define MATERIAL_UPPER_LIMIT 32

groupshared static uint groupTotalSize = GROUP_SIZE_X * GROUP_SIZE_Y;

[numthreads(GROUP_SIZE_X, GROUP_SIZE_Y, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint GTid : SV_GroupIndex)
{
    float w;
    float h;
    visibilityBuffer.GetDimensions(w, h);

    if (DTid.x >= w || DTid.y >= h)
    {
        return;
    }
    
    uint packedvis = visibilityBuffer[DTid.xy];
    uint drawcallId = packedvis >> NUM_TRIANGLE_BITS;
    uint materialId = offsetBuffer[drawcallId].r;
    
    float2 texCoord = float2((float) DTid.x / w, (float) DTid.y / h);
    
    for (uint i = 0; i < MATERIAL_UPPER_LIMIT; ++i)
    {
        bool addTexcoord = i == materialId;
        uint laneOffset = WavePrefixCountBits(addTexcoord);
        uint count = WaveActiveCountBits(addTexcoord);
        if (addTexcoord)
        {
            uint appendOffset;
            if (WaveIsFirstLane())
            {
                InterlockedAdd(materialCountBuffer[materialId], count, appendOffset);
            }

            appendOffset = WaveReadLaneFirst(appendOffset);
            appendOffset += laneOffset;
            pixelPositionBuffer[materialOffsetBuffer[materialId] + appendOffset] = texCoord;
        }
    }
}


