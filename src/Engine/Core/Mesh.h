#pragma once

#include <DirectXMath.h>       // For XMFLOAT3, XMFLOAT2

#include <d3d12.h>  // For D3D12_INPUT_LAYOUT_DESC, D3D12_INPUT_ELEMENT_DESC

#include <map>     // For std::map
#include <memory>  // For std::shared_ptr

#include <Engine/Core/VertexTypes.h>

class CommandList;
class IndexBuffer;
class Material;
class VertexBuffer;
class Visitor;

struct MeshPrimitive
{
    RefPtr<VertexBuffer> m_VertexBuffer;
    RefPtr<IndexBuffer> m_IndexBuffer;

    std::vector<VertexPositionNormalTangentBitangentTexture> m_VertexContainer;
    std::vector<u32> m_IndexContainer;

    u32 m_MaterialIndex;

    MeshPrimitive() = default;

    MeshPrimitive(const RefPtr<VertexBuffer>& vertexBuffer, const RefPtr<IndexBuffer>& indexBuffer, u32 materialIndex)
        : m_VertexBuffer(vertexBuffer)
        , m_IndexBuffer(indexBuffer)
        , m_MaterialIndex(materialIndex)
    {}

    void                          SetVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer);
    std::shared_ptr<VertexBuffer> GetVertexBuffer() const;

    void                         SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer);
    std::shared_ptr<IndexBuffer> GetIndexBuffer();
};

class Mesh
{
public:
    Mesh();
    ~Mesh() = default;

    void                     SetPrimitiveTopology( D3D12_PRIMITIVE_TOPOLOGY primitiveToplogy );
    D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const;

    const std::vector<MeshPrimitive>& GetPrimitives() const { return m_Primitives; }

    MeshPrimitive& CreatePrimitive(const RefPtr<VertexBuffer>& vertexBuffer, const RefPtr<IndexBuffer>& indexBuffer, u32 materialIndex = ~0) 
    {
        return m_Primitives.emplace_back(vertexBuffer, indexBuffer, materialIndex); 
    }

    /**
     * Get the number if indices in the index buffer.
     * If no index buffer is bound to the mesh, this function returns 0.
     */
    size_t GetIndexCount() const;

    /**
     * Get the number of vertices in the mesh.
     * If this mesh does not have a vertex buffer, the function returns 0.
     */
    size_t GetVertexCount() const;

    void                      SetMaterials( std::vector<RefPtr<Material>> const& material );
    const std::vector<RefPtr<Material>>& GetMaterials() const;

private:
    friend class Renderer;
    friend class MeshAssetHandler;

    std::vector<MeshPrimitive>   m_Primitives;
    std::vector<RefPtr<Material>> m_Materials;
    //std::shared_ptr<IndexBuffer> m_IndexBuffer;
    //std::shared_ptr<Material>    m_Material;
    D3D12_PRIMITIVE_TOPOLOGY     m_PrimitiveTopology;
};
