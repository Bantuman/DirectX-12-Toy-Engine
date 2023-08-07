#pragma once

#include "Buffer.h"
#include "ByteAddressBuffer.h"

class Device;

class StructuredBuffer : public Buffer
{

public:
    /**
     * Get the number of elements contained in this buffer.
     */
    virtual size_t GetNumElements() const
    {
        return m_NumElements;
    }

    /**
     * Get the size in bytes of each element in this buffer.
     */
    virtual size_t GetElementSize() const
    {
        return m_ElementSize;
    }

    std::shared_ptr<ByteAddressBuffer> GetCounterBuffer() const
    {
        return m_CounterBuffer;
    }

    StructuredBuffer( Device& device, size_t numElements,
                      size_t elementSize );
    StructuredBuffer( Device& device, Microsoft::WRL::ComPtr<ID3D12Resource> resource,
                      size_t numElements, size_t elementSize );

    virtual ~StructuredBuffer() = default;

private:
    size_t m_NumElements;
    size_t m_ElementSize;

    // A buffer to store the internal counter for the structured buffer.
    RefPtr<ByteAddressBuffer> m_CounterBuffer;
};
