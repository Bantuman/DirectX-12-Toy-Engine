/**
 * Index buffer resource.
 */
#pragma once

#include "Buffer.h"

class IndexBuffer : public Buffer
{
public:
    /**
     * Get the index buffer view for biding to the Input Assembler stage.
     */
    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const
    {
        return m_IndexBufferView;
    }

    size_t GetNumIndices() const
    {
        return m_NumIndices;
    }

    DXGI_FORMAT GetIndexFormat() const
    {
        return m_IndexFormat;
    }

    IndexBuffer(Device& device, size_t numIndices, DXGI_FORMAT indexFormat);
    IndexBuffer(Device& device, Microsoft::WRL::ComPtr<ID3D12Resource> resource, size_t numIndices,
        DXGI_FORMAT indexFormat);
    virtual ~IndexBuffer() = default;

    void CreateIndexBufferView();

private:
    size_t      m_NumIndices;
    DXGI_FORMAT m_IndexFormat;
    D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
};