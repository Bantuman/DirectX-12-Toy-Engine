#pragma once

#include "DescriptorAllocation.h"

#include <map>
#include <mutex>
#include <queue>

class Device;

class DescriptorAllocatorPage : public std::enable_shared_from_this<DescriptorAllocatorPage>
{
public:
    D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const;

    /**
        * Check to see if this descriptor page has a contiguous block of descriptors
        * large enough to satisfy the request.
        */
    bool HasSpace(u32 numDescriptors) const;

    /**
        * Get the number of available handles in the heap.
        */
    u32 NumFreeHandles() const;

    /**
        * Allocate a number of descriptors from this descriptor heap.
        * If the allocation cannot be satisfied, then a NULL descriptor
        * is returned.
        */
    DescriptorAllocation Allocate(u32 numDescriptors);

    /**
        * Return a descriptor back to the heap.
        * @param frameNumber Stale descriptors are not freed directly, but put
        * on a stale allocations queue. Stale allocations are returned to the heap
        * using the DescriptorAllocatorPage::ReleaseStaleAllocations method.
        */
    void Free(DescriptorAllocation&& descriptorHandle);

    /**
        * Returned the stale descriptors back to the descriptor heap.
        */
    void ReleaseStaleDescriptors();

    DescriptorAllocatorPage(Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, u32 numDescriptors);
    virtual ~DescriptorAllocatorPage() = default;
protected:

    // Compute the offset of the descriptor handle from the start of the heap.
    u32 ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle);

    // Adds a new block to the free list.
    void AddNewBlock(u32 offset, u32 numDescriptors);

    // Free a block of descriptors.
    // This will also merge free blocks in the free list to form larger blocks
    // that can be reused.
    void FreeBlock(u32 offset, u32 numDescriptors);

private:
    // The offset (in descriptors) within the descriptor heap.
    using OffsetType = u32;
    // The number of descriptors that are available.
    using SizeType = u32;

    struct FreeBlockInfo;
    // A map that lists the free blocks by the offset within the descriptor heap.
    using FreeListByOffset = std::map<OffsetType, FreeBlockInfo>;

    // A map that lists the free blocks by size.
    // Needs to be a multimap since multiple blocks can have the same size.
    using FreeListBySize = std::multimap<SizeType, FreeListByOffset::iterator>;

    struct FreeBlockInfo
    {
        FreeBlockInfo(SizeType size)
            : Size(size)
        {}

        SizeType                 Size;
        FreeListBySize::iterator FreeListBySizeIt;
    };

    struct StaleDescriptorInfo
    {
        StaleDescriptorInfo(OffsetType offset, SizeType size)
            : Offset(offset)
            , Size(size)
        {}

        // The offset within the descriptor heap.
        OffsetType Offset;
        // The number of descriptors
        SizeType Size;
    };

    // Device that was used to create the descriptor heap.
    Device& m_Device;

    // Stale descriptors are queued for release until the frame that they were freed
    // has completed.
    using StaleDescriptorQueue = std::queue<StaleDescriptorInfo>;

    FreeListByOffset     m_FreeListByOffset;
    FreeListBySize       m_FreeListBySize;
    StaleDescriptorQueue m_StaleDescriptors;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_d3d12DescriptorHeap;
    D3D12_DESCRIPTOR_HEAP_TYPE                   m_HeapType;
    CD3DX12_CPU_DESCRIPTOR_HANDLE                m_BaseDescriptor;
    u32                                     m_DescriptorHandleIncrementSize;
    u32                                     m_NumDescriptorsInHeap;
    u32                                     m_NumFreeHandles;

    std::mutex m_AllocationMutex;
};
