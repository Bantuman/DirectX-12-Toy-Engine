/**
 * Vertex buffer resource.
 */
#pragma once

#include "Buffer.h"

class VertexBuffer : public Buffer
{
public:
    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const
    {
        return m_VertexBufferView;
    }

    size_t GetNumVertices() const
    {
        return m_NumVertices;
    }

    size_t GetVertexStride() const
    {
        return m_VertexStride;
    }

    VertexBuffer(Device& device, size_t numVertices, size_t vertexStride);
    VertexBuffer(Device& device, Microsoft::WRL::ComPtr<ID3D12Resource> resource, size_t numVertices,
        size_t vertexStride);
    virtual ~VertexBuffer();

    void CreateVertexBufferView();

private:
    size_t                   m_NumVertices;
    size_t                   m_VertexStride;
    D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
};