#pragma once

#include "Buffer.h"
#include <Memory/Descriptors/DescriptorAllocation.h>

class Device;

class ByteAddressBuffer : public Buffer
{
public:
    size_t GetBufferSize() const
    {
        return m_BufferSize;
    }

    ByteAddressBuffer( Device& device, const D3D12_RESOURCE_DESC& resDesc );
    ByteAddressBuffer( Device& device, Microsoft::WRL::ComPtr<ID3D12Resource> resource );
    virtual ~ByteAddressBuffer() = default;

private:
    size_t m_BufferSize;
};
