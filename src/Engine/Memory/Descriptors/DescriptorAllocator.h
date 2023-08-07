#include "DescriptorAllocation.h"

#include <mutex>

class DescriptorAllocatorPage;
class Device;

class DescriptorAllocator
{
public:
    /**
     * Allocate a number of contiguous descriptors from a CPU visible descriptor heap.
     *
     * @param numDescriptors The number of contiguous descriptors to allocate.
     * Cannot be more than the number of descriptors per descriptor heap.
     */
    DescriptorAllocation Allocate(u32 numDescriptors = 1);

    /**
     * When the frame has completed, the stale descriptors can be released.
     */
    void ReleaseStaleDescriptors();

    // Can only be created by the Device.
    DescriptorAllocator(Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, u32 numDescriptorsPerHeap = 256);
    virtual ~DescriptorAllocator();

protected:
    friend struct std::default_delete<DescriptorAllocator>;

private:
    using DescriptorHeapPool = std::vector<std::shared_ptr<DescriptorAllocatorPage>>;

    // Create a new heap with a specific number of descriptors.
    std::shared_ptr<DescriptorAllocatorPage> CreateAllocatorPage();

    // The device that was use to create this DescriptorAllocator.
    Device& m_Device;
    D3D12_DESCRIPTOR_HEAP_TYPE m_HeapType;
    u32                   m_NumDescriptorsPerHeap;

    DescriptorHeapPool m_HeapPool;

    // Indices of available heaps in the heap pool.
    std::set<size_t> m_AvailableHeaps;

    std::mutex m_AllocationMutex;
};
