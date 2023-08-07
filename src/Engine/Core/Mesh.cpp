#include "enginepch.h"

#include <Pipeline/CommandList.h>
#include <Buffers/IndexBuffer.h>
#include <Buffers/VertexBuffer.h>
#include "Mesh.h"
#include "Material.h"

#include <Engine/Asset/MaterialAssetHandler.h>

Mesh::Mesh()
: m_PrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST )
{}

void Mesh::SetPrimitiveTopology( D3D12_PRIMITIVE_TOPOLOGY primitiveToplogy )
{
    m_PrimitiveTopology = primitiveToplogy;
}

D3D12_PRIMITIVE_TOPOLOGY Mesh::GetPrimitiveTopology() const
{
    return m_PrimitiveTopology;
}

void MeshPrimitive::SetVertexBuffer( const std::shared_ptr<VertexBuffer>& vertexBuffer )
{
    m_VertexBuffer = vertexBuffer;
}

std::shared_ptr<VertexBuffer> MeshPrimitive::GetVertexBuffer() const
{
   // auto iter         = m_VertexBuffers.find( slotID );
   // auto vertexBuffer = iter != m_VertexBuffers.end() ? iter->second : nullptr;

    return m_VertexBuffer;
}

void MeshPrimitive::SetIndexBuffer( const std::shared_ptr<IndexBuffer>& indexBuffer )
{
    m_IndexBuffer = indexBuffer;
}

std::shared_ptr<IndexBuffer> MeshPrimitive::GetIndexBuffer()
{
    return m_IndexBuffer;
}

size_t Mesh::GetIndexCount() const
{
    size_t indexCount = 0;
   
    for (auto& primitive : m_Primitives)
    {
        indexCount += primitive.m_IndexBuffer->GetNumIndices();
    }

    return indexCount;
}

size_t Mesh::GetVertexCount() const
{
    size_t vertexCount = 0;

    for (auto& primitive : m_Primitives)
    {
        vertexCount += primitive.m_VertexBuffer->GetNumVertices();
    }
    
    return vertexCount;
}

void Mesh::SetMaterials( std::vector<RefPtr<Material>> const& material )
{
    m_Materials = material;
}

std::vector<RefPtr<Material>> const& Mesh::GetMaterials() const
{
    return m_Materials;
}
