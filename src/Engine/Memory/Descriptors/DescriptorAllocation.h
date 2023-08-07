#pragma once

#include <Engine/Core/Defines.h>

class DescriptorAllocatorPage;

class DescriptorAllocation
{
public:
    // Creates a NULL descriptor.
    DescriptorAllocation();

    DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, u32 numHandles, u32 descriptorSize,
        std::shared_ptr<DescriptorAllocatorPage> page);

    // The destructor will automatically free the allocation.
    ~DescriptorAllocation();

    // Copies are not allowed.
    DescriptorAllocation(const DescriptorAllocation&) = delete;
    DescriptorAllocation& operator=(const DescriptorAllocation&) = delete;

    // Move is allowed.
    DescriptorAllocation(DescriptorAllocation&& allocation) noexcept;
    DescriptorAllocation& operator=(DescriptorAllocation&& other) noexcept;

    // Check if this a valid descriptor.
    bool IsNull() const;
    bool IsValid() const
    {
        return !IsNull();
    }

    // Get a descriptor at a particular offset in the allocation.
    D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle(u32 offset = 0) const;

    // Get the number of (consecutive) handles for this allocation.
    u32 GetNumHandles() const;

    // Get the heap that this allocation came from.
    // (For internal use only).
    std::shared_ptr<DescriptorAllocatorPage> GetDescriptorAllocatorPage() const;

private:
    // Free the descriptor back to the heap it came from.
    void Free();

    // The base descriptor.
    D3D12_CPU_DESCRIPTOR_HANDLE m_Descriptor;
    // The number of descriptors in this allocation.
    u32 m_NumHandles;
    // The offset to the next descriptor.
    u32 m_DescriptorSize;

    // A pointer back to the original page where this allocation came from.
    std::shared_ptr<DescriptorAllocatorPage> m_Page;
};
