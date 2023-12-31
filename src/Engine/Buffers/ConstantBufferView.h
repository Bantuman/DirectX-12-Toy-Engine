#pragma once

#include <Engine/Memory/Descriptors/DescriptorAllocation.h>

class ConstantBuffer;
class Device;

class ConstantBufferView
{
public:
    std::shared_ptr<ConstantBuffer> GetConstantBuffer() const
    {
        return m_ConstantBuffer;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle()
    {
        return m_Descriptor.GetDescriptorHandle();
    }

    ConstantBufferView( Device& device, const std::shared_ptr<ConstantBuffer>& constantBuffer,
                        size_t offset = 0 );
    virtual ~ConstantBufferView() = default;

private:
    Device&                         m_Device;
    std::shared_ptr<ConstantBuffer> m_ConstantBuffer;
    DescriptorAllocation            m_Descriptor;
};
