#pragma once

#include <Engine/Memory/Descriptors/DescriptorAllocation.h>

class Device;
class Resource;

class UnorderedAccessView
{
public:
    std::shared_ptr<Resource> GetResource() const
    {
        return m_Resource;
    }

    std::shared_ptr<Resource> GetCounterResource() const
    {
        return m_CounterResource;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle() const
    {
        return m_Descriptor.GetDescriptorHandle();
    }

    UnorderedAccessView( Device& device, const std::shared_ptr<Resource>& resource,
                         const std::shared_ptr<Resource>&        counterResource = nullptr,
                         const D3D12_UNORDERED_ACCESS_VIEW_DESC* uav             = nullptr );
    virtual ~UnorderedAccessView() = default;

private:
    Device&                   m_Device;
    std::shared_ptr<Resource> m_Resource;
    std::shared_ptr<Resource> m_CounterResource;
    DescriptorAllocation      m_Descriptor;
};
