#include "../CommonStructs.hlsli"

struct IndirectArgs
{
    uint Offset;
    uint Count;
    uint3 Invocations;
};

RWStructuredBuffer<uint> materialCountBuffer : register(u0);
RWStructuredBuffer<uint> materialOffsetBuffer : register(u1);
RWStructuredBuffer<IndirectArgs> indirectArguementBuffer : register(u2);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint offset = 0;
    for (uint i = 0; i < 96; ++i)
    {
        uint fragcount = materialCountBuffer[i];
        uint originalValue;
        InterlockedExchange(materialOffsetBuffer[i], offset, originalValue);
        indirectArguementBuffer[i].Invocations = uint3((max(fragcount, 256) / 256) + 1, 1, 1);
        indirectArguementBuffer[i].Count = fragcount;
        indirectArguementBuffer[i].Offset = offset;
        offset += fragcount;
        InterlockedExchange(materialCountBuffer[i], 0, originalValue);
    }
}


