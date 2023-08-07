/**
 * Constant buffer resource.
 */
#pragma once

#include "Buffer.h"
#include <Memory/Descriptors/DescriptorAllocation.h>

class ConstantBuffer : public Buffer
{
public:

    size_t GetSizeInBytes() const
    {
        return m_SizeInBytes;
    }

    ConstantBuffer(Device& device, Microsoft::WRL::ComPtr<ID3D12Resource> resource);
    virtual ~ConstantBuffer();

private:
    size_t               m_SizeInBytes;
};