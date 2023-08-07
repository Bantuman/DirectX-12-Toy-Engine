#pragma once

#include <Engine/Core/VertexTypes.h>

#include <functional>  // For std::function
#include <map>         // for std::map
#include <mutex>       // for std::mutex

class Buffer;
class ByteAddressBuffer;
class ConstantBuffer;
class ConstantBufferView;
class Device;
class DynamicDescriptorHeap;
class GenerateMipsPSO;
class IndexBuffer;
class PanoToCubemapPSO;
class PipelineStateObject;
class RenderTarget;
class Resource;
class ResourceStateTracker;
class RootSignature;
class ShaderResourceView;
class StructuredBuffer;
class Texture;
class UnorderedAccessView;
class UploadBuffer;
class VertexBuffer;
class Scene;

class CommandList : public std::enable_shared_from_this<CommandList>
{
public:
    /**
        * Get the type of command list.
        */
    D3D12_COMMAND_LIST_TYPE GetCommandListType() const
    {
        return m_d3d12CommandListType;
    }

    /**
        * Get the device that was used to create this command list.
        */
    Device& GetDevice() const
    {
        return m_Device;
    }

    void CommitStagedDescriptorsForDispatch();


    /**
        * Get direct access to the ID3D12GraphicsCommandList2 interface.
        */
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> GetD3D12CommandList() const
    {
        return m_d3d12CommandList;
    }

    /**
        * Transition a resource to a particular state.
        *
        * @param resource The resource to transition.
        * @param stateAfter The state to transition the resource to. The before state is resolved by the resource state
        * tracker.
        * @param subresource The subresource to transition. By default, this is D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES
        * which indicates that all subresources are transitioned to the same state.
        * @param flushBarriers Force flush any barriers. Resource barriers need to be flushed before a command (draw,
        * dispatch, or copy) that expects the resource to be in a particular state can run.
        */
    void TransitionBarrier(const std::shared_ptr<Resource>& resource, D3D12_RESOURCE_STATES stateAfter,
        UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false);
    void TransitionBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES stateAfter,
        UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false);

    /**
        * Add a UAV barrier to ensure that any writes to a resource have completed
        * before reading from the resource.
        *
        * @param [resource} The resource to add a UAV barrier for (can be null).
        * @param flushBarriers Force flush any barriers. Resource barriers need to be
        * flushed before a command (draw, dispatch, or copy) that expects the resource
        * to be in a particular state can run.
        */
    void UAVBarrier(const std::shared_ptr<Resource>& resource = nullptr, bool flushBarriers = false);
    void UAVBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> resource, bool flushBarriers = false);

    /**
        * Add an aliasing barrier to indicate a transition between usages of two
        * different resources that occupy the same space in a heap.
        *
        * @param [beforeResource] The resource that currently occupies the heap (can be null).
        * @param [afterResource] The resource that will occupy the space in the heap (can be null).
        */
    void AliasingBarrier(const std::shared_ptr<Resource> & = nullptr,
        const std::shared_ptr<Resource>& afterResource = nullptr, bool flushBarriers = false);
    void AliasingBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> beforeResource,
        Microsoft::WRL::ComPtr<ID3D12Resource> afterResource, bool flushBarriers = false);

    /**
        * Flush any barriers that have been pushed to the command list.
        */
    void FlushResourceBarriers();

    /**
        * Copy resources.
        */
    void CopyResource(const std::shared_ptr<Resource>& dstRes, const std::shared_ptr<Resource>& srcRes);
    void CopyResource(Microsoft::WRL::ComPtr<ID3D12Resource> dstRes, Microsoft::WRL::ComPtr<ID3D12Resource> srcRes);

    /**
        * Resolve a multisampled resource into a non-multisampled resource.
        */
    void ResolveSubresource(const std::shared_ptr<Resource>&, const std::shared_ptr<Resource>&,
        u32 dstSubresource = 0, u32 srcSubresource = 0);

    /**
        * Copy the contents to a vertex buffer in GPU memory.
        */
    std::shared_ptr<VertexBuffer> CopyVertexBuffer(size_t numVertices, size_t vertexStride,
        const void* vertexBufferData);
    template<typename T>
    std::shared_ptr<VertexBuffer> CopyVertexBuffer(const std::vector<T>& vertexBufferData)
    {
        return CopyVertexBuffer(vertexBufferData.size(), sizeof(T), vertexBufferData.data());
    }

    /**
        * Copy the contents to a index buffer in GPU memory.
        */
    std::shared_ptr<IndexBuffer> CopyIndexBuffer(size_t numIndices, DXGI_FORMAT indexFormat,
        const void* indexBufferData);
    template<typename T>
    std::shared_ptr<IndexBuffer> CopyIndexBuffer(const std::vector<T>& indexBufferData)
    {
        assert(sizeof(T) == 2 || sizeof(T) == 4);

        DXGI_FORMAT indexFormat = (sizeof(T) == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
        return CopyIndexBuffer(indexBufferData.size(), indexFormat, indexBufferData.data());
    }

    /**
        * Copy the contents to a constant buffer in GPU memory.
        */
    std::shared_ptr<ConstantBuffer> CopyConstantBuffer(size_t bufferSize, const void* bufferData);

    template<typename T>
    std::shared_ptr<ConstantBuffer> CopyConstantBuffer(const T& data)
    {
        return CopyConstantBuffer(sizeof(T), &data);
    }

    /**
        * Copy the contents to a byte address buffer in GPU memory.
        */
    std::shared_ptr<ByteAddressBuffer> CopyByteAddressBuffer(size_t bufferSize, const void* bufferData);
    template<typename T>
    std::shared_ptr<ByteAddressBuffer> CopyByteAddressBuffer(const T& data)
    {
        return CopyByteAddressBuffer(sizeof(T), &data);
    }

    /**
        * Copy the contents to a structured buffer in GPU memory.
        */
    std::shared_ptr<StructuredBuffer> CopyStructuredBuffer(size_t numElements, size_t elementSize,
        const void* bufferData);
    template<typename T>
    std::shared_ptr<StructuredBuffer> CopyStructuredBuffer(const std::vector<T>& bufferData)
    {
        return CopyStructuredBuffer(bufferData.size(), sizeof(T), bufferData.data());
    }

    /**
        * Set the current primitive topology for the rendering pipeline.
        */
    void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitiveTopology);

    /**
        * Load a texture by a filename.
        */
   // std::shared_ptr<Texture> LoadTextureFromFile(const std::wstring& fileName, bool sRGB = false);

    /**
        * Create a cube.
        *
        * @param size The size of one side of the cube.
        * @param reverseWinding Whether to reverse the winding order of the triangles (useful for skyboxes).
        */
    std::shared_ptr<Scene> CreateCube(float size = 1.0, bool reverseWinding = false);

    /**
        * Create a sphere.
        *
        * @param radius Radius of the sphere.
        * @param tessellation Determines how smooth the sphere is.
        * @param reverseWinding Whether to reverse the winding order of the triangles (useful for sydomes).
        */
    std::shared_ptr<Scene> CreateSphere(float radius = 0.5f, u32 tessellation = 16, bool reversWinding = false);

    /**
        * Create a Cylinder
        *
        * @param radius The radius of the primary axis of the cylinder.
        * @param hight The height of the cylinder.
        * @param tessellation How smooth the cylinder will be.
        * @param reverseWinding Whether to reverse the winding order of the triangles.
        */
    std::shared_ptr<Scene> CreateCylinder(float radius = 0.5f, float height = 1.0f, u32 tessellation = 32,
        bool reverseWinding = false);

    /**
        * Create a cone.
        *
        * @param radius The radius of the base of the cone.
        * @param height The height of the cone.
        * @param tessellation How smooth to make the cone.
        * @param reverseWinding Whether to reverse the winding order of the triangles.
        */
    std::shared_ptr<Scene> CreateCone(float radius = 0.5f, float height = 1.0f, u32 tessellation = 32,
        bool reverseWinding = false);

    /**
        * Create a torus.
        *
        * @param radius The radius of the torus.
        * @param thickness The The thickness of the torus.
        * @param tessellation The smoothness of the torus.
        * @param reverseWinding Reverse the winding order of the vertices.
        */
    std::shared_ptr<Scene> CreateTorus(float radius = 0.5f, float thickness = 0.333f, u32 tessellation = 32,
        bool reverseWinding = false);

    /**
        * Create a plane.
        *
        * @param width The width of the plane.
        * @param height The height of the plane.
        * @reverseWinding Whether to reverse the winding order of the plane.
        */
    std::shared_ptr<Scene> CreatePlane(float width = 1.0f, float height = 1.0f, bool reverseWinding = false);

    /**
    * Clear a UAV.
    */
    void ClearUnorderedAccessViewUint(const std::shared_ptr<UnorderedAccessView>& view, const u32 values[4]);

    /**
    * Clear a UAV.
    */
    void ClearUnorderedAccessViewFloat(const std::shared_ptr<UnorderedAccessView>& view, const float value[4]);

    /**
        * Clear a texture.
        */
    void ClearTexture(const std::shared_ptr<Texture>& texture, const float clearColor[4]);

    /**
        * Clear depth/stencil texture.
        */
    void ClearDepthStencilTexture(const std::shared_ptr<Texture>& texture, D3D12_CLEAR_FLAGS clearFlags,
        float depth = 1.0f, u8 stencil = 0);

    /**
        * Generate mips for the texture.
        * The first subresource is used to generate the mip chain.
        * Mips are automatically generated for textures loaded from files.
        */
    void GenerateMips(const std::shared_ptr<Texture>& texture);

    /**
        * Generate a cubemap texture from a panoramic (equirectangular) texture.
        */
    void PanoToCubemap(const std::shared_ptr<Texture>& cubemapTexture, const std::shared_ptr<Texture>& panoTexture);

    /**
        * Copy subresource data to a texture.
        */
    void CopyTextureSubresource(const std::shared_ptr<Texture>& texture, u32 firstSubresource,
        u32 numSubresources, D3D12_SUBRESOURCE_DATA* subresourceData);

    /**
        * Set a dynamic constant buffer data to an inline descriptor in the root
        * signature.
        */
    void SetGraphicsDynamicConstantBuffer(u32 rootParameterIndex, size_t sizeInBytes, const void* bufferData);
    template<typename T>
    void SetGraphicsDynamicConstantBuffer(u32 rootParameterIndex, const T& data)
    {
        SetGraphicsDynamicConstantBuffer(rootParameterIndex, sizeof(T), &data);
    }

    /**
        * Set a set of 32-bit constants on the graphics pipeline.
        */
    void SetGraphics32BitConstants(u32 rootParameterIndex, u32 numConstants, const void* constants);
    template<typename T>
    void SetGraphics32BitConstants(u32 rootParameterIndex, const T& constants)
    {
        static_assert(sizeof(T) % sizeof(u32) == 0, "Size of type must be a multiple of 4 bytes");
        SetGraphics32BitConstants(rootParameterIndex, sizeof(T) / sizeof(u32), &constants);
    }

    void SetCompute32BitConstants(u32 rootParameterIndex, u32 numConstants, const void* constants);
    template<typename T>
    void SetCompute32BitConstants(u32 rootParameterIndex, const T& constants)
    {
        static_assert(sizeof(T) % sizeof(u32) == 0, "Size of type must be a multiple of 4 bytes");
        SetCompute32BitConstants(rootParameterIndex, sizeof(T) / sizeof(u32), &constants);
    }

    void SetVertexBuffers(u32 startSlot, const std::vector<std::shared_ptr<VertexBuffer>>& vertexBufferViews);
    void SetVertexBuffer(u32 slot, const std::shared_ptr<VertexBuffer>& vertexBufferView);

    void SetDynamicVertexBuffer(u32 slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData);
    template<typename T>
    void SetDynamicVertexBuffer(u32 slot, const std::vector<T>& vertexBufferData)
    {
        SetDynamicVertexBuffer(slot, vertexBufferData.size(), sizeof(T), vertexBufferData.data());
    }
    void SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer);

    void SetDynamicIndexBuffer(size_t numIndices, DXGI_FORMAT indexFormat, const void* indexBufferData);
    template<typename T>
    void SetDynamicIndexBuffer(const std::vector<T>& indexBufferData)
    {
        static_assert(sizeof(T) == 2 || sizeof(T) == 4);

        DXGI_FORMAT indexFormat = (sizeof(T) == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
        SetDynamicIndexBuffer(indexBufferData.size(), indexFormat, indexBufferData.data());
    }
    void SetGraphicsDynamicStructuredBuffer(u32 slot, size_t numElements, size_t elementSize,
        const void* bufferData);
    template<typename T>
    void SetGraphicsDynamicStructuredBuffer(u32 slot, const std::vector<T>& bufferData)
    {
        SetGraphicsDynamicStructuredBuffer(slot, bufferData.size(), sizeof(T), bufferData.data());
    }

    void SetComputeDynamicStructuredBuffer(u32 slot, size_t numElements, size_t elementSize,
        const void* bufferData);
    template<typename T>
    void SetComputeDynamicStructuredBuffer(u32 slot, const std::vector<T>& bufferData)
    {
        SetComputeDynamicStructuredBuffer(slot, bufferData.size(), sizeof(T), bufferData.data());
    }

    void SetViewport(const D3D12_VIEWPORT& viewport);
    void SetViewports(const std::vector<D3D12_VIEWPORT>& viewports);

    void SetScissorRect(const D3D12_RECT& scissorRect);
    void SetScissorRects(const std::vector<D3D12_RECT>& scissorRects);

    void SetPipelineState(const std::shared_ptr<PipelineStateObject>& pipelineState);

    void SetGraphicsRootSignature(const std::shared_ptr<RootSignature>& rootSignature);
    void SetComputeRootSignature(const std::shared_ptr<RootSignature>& rootSignature);

    /**
        * Set an inline CBV.
        *
        * Note: Only ConstantBuffer's can be used with inline CBV's.
        */
    void SetConstantBufferView(u32 rootParameterIndex, const std::shared_ptr<ConstantBuffer>& buffer,
        D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
        size_t                bufferOffset = 0);

    /**
        * Set an inline SRV.
        *
        * Note: Only Buffer resources can be used with inline SRV's
        */
    void SetShaderResourceView(u32 rootParameterIndex, const std::shared_ptr<Buffer>& buffer,
        D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        size_t bufferOffset = 0);

    void SetShaderResourceViews(u32 rootParameterIndex, const std::shared_ptr<Buffer>& buffer,
        D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        size_t bufferOffset = 0);
    /**
        * Set an inline UAV.
        *
        * Note: Only Buffer resoruces can be used with inline UAV's.
        */
    void SetUnorderedAccessView(u32 rootParameterIndex, const std::shared_ptr<Buffer>& buffer,
        D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        size_t                bufferOffset = 0);

    /**
        * Set the CBV on the rendering pipeline.
        */
    void SetConstantBufferView(u32 rootParameterIndex, u32 descriptorOffset,
        const std::shared_ptr<ConstantBufferView>& cbv,
        D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    /**
        * Set the SRV on the graphics pipeline.
        */
    void SetShaderResourceView(u32 rootParameterIndex, u32 descriptorOffset,
        const std::shared_ptr<ShaderResourceView>& srv,
        D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        UINT firstSubresource = 0,
        UINT numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

    /**
        * Set an SRV on the graphics pipeline using the default SRV for the texture.
        */
    void SetShaderResourceView(int32_t rootParameterIndex, u32 descriptorOffset,
        const std::shared_ptr<Texture>& texture,
        D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        UINT firstSubresource = 0,
        UINT numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    /**
        * Set the UAV on the graphics pipeline.
        */
    void SetUnorderedAccessView(u32 rootParameterIndex, u32 descriptorOffset,
        const std::shared_ptr<UnorderedAccessView>& uav,
        D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        UINT                  firstSubresource = 0,
        UINT                  numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

    /**
        * Set the UAV on the graphics pipline using a specific mip of the texture.
        */
    void SetUnorderedAccessView(u32 rootParameterIndex, u32 descriptorOffset,
        const std::shared_ptr<Texture>& texture, UINT mip,
        D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        UINT                  firstSubresource = 0,
        UINT                  numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

    /**
        * Set the render targets for the graphics rendering pipeline.
        */
    void SetRenderTarget(const RenderTarget& renderTarget);

    /**
        * Draw geometry.
        */
    void Draw(u32 vertexCount, u32 instanceCount = 1, u32 startVertex = 0, u32 startInstance = 0);
    void DrawIndexed(u32 indexCount, u32 instanceCount = 1, u32 startIndex = 0, int32_t baseVertex = 0,
        u32 startInstance = 0);

    /**
        * Dispatch a compute shader.
        */
    void Dispatch(u32 numGroupsX, u32 numGroupsY = 1, u32 numGroupsZ = 1);

    CommandList(Device& device, D3D12_COMMAND_LIST_TYPE type);
    virtual ~CommandList();

protected:
    friend class TextureAssetHandler;
    friend class CommandQueue;
    friend class DynamicDescriptorHeap;
    friend struct std::default_delete<CommandList>;

    /**
        * Close the command list.
        * Used by the command queue.
        *
        * @param pendingCommandList The command list that is used to execute pending
        * resource barriers (if any) for this command list.
        *
        * @return true if there are any pending resource barriers that need to be
        * processed.
        */
    bool Close(const std::shared_ptr<CommandList>& pendingCommandList);
    // Just close the command list. This is useful for pending command lists.
    void Close();

    /**
        * Reset the command list. This should only be called by the CommandQueue
        * before the command list is returned from CommandQueue::GetCommandList.
        */
    void Reset();

    /**
        * Release tracked objects. Useful if the swap chain needs to be resized.
        */
    void ReleaseTrackedObjects();

    /**
        * Set the currently bound descriptor heap.
        * Should only be called by the DynamicDescriptorHeap class.
        */
    void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap);

    std::shared_ptr<CommandList> GetGenerateMipsCommandList() const
    {
        return m_ComputeCommandList;
    }

private:
    // Used for procedural mesh generation.
    using VertexCollection = std::vector<VertexPositionNormalTangentBitangentTexture>;
    using IndexCollection = std::vector<uint16_t>;

    // Create a scene that contains a single node with a single mesh.
    std::shared_ptr<Scene> CreateScene(const VertexCollection& vertices, const IndexCollection& indices);

    // Helper function for flipping winding of geometric primitives for LH vs. RH coords
    inline void ReverseWinding(IndexCollection& indices, VertexCollection& vertices);
    // Helper function for inverting normals for "inside" vs "outside" viewing.
    inline void InvertNormals(VertexCollection& vertices);
    // Helper function to compute a point on a unit circle aligned to the x,z plane and centered at the origin.
    inline DirectX::XMVECTOR GetCircleVector(size_t i, size_t tessellation) noexcept;
    // Helper function to compute a tangent vector at a point on a unit sphere aligned to the x,z plane.
    inline DirectX::XMVECTOR GetCircleTangent(size_t i, size_t tessellation) noexcept;
    // Helper creates a triangle fan to close the end of a cylinder / cone
    void CreateCylinderCap(VertexCollection& vertices, IndexCollection& indices, size_t tessellation, float height,
        float radius, bool isTop);

    // Add a resource to a list of tracked resources (ensures lifetime while command list is in-flight on a command
    // queue.
    void TrackResource(Microsoft::WRL::ComPtr<ID3D12Object> object);
    void TrackResource(const std::shared_ptr<Resource>& res);

    // Generate mips for UAV compatible textures.
    void GenerateMips_UAV(const std::shared_ptr<Texture>& texture, bool isSRGB);

    // Copy the contents of a CPU buffer to a GPU buffer (possibly replacing the previous buffer contents).
    Microsoft::WRL::ComPtr<ID3D12Resource> CopyBuffer(size_t bufferSize, const void* bufferData,
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

    // Binds the current descriptor heaps to the command list.
    void BindDescriptorHeaps();

    // The device that is used to create this command list.
    Device& m_Device;
    D3D12_COMMAND_LIST_TYPE                            m_d3d12CommandListType;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> m_d3d12CommandList;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>     m_d3d12CommandAllocator;

    // For copy queues, it may be necessary to generate mips while loading textures.
    // Mips can't be generated on copy queues but must be generated on compute or
    // direct queues. In this case, a Compute command list is generated and executed
    // after the copy queue is finished uploading the first sub resource.
    std::shared_ptr<CommandList> m_ComputeCommandList;

    // Keep track of the currently bound root signatures to minimize root
    // signature changes.
    ID3D12RootSignature* m_RootSignature;
    // Keep track of the currently bond pipeline state object to minimize PSO changes.
    ID3D12PipelineState* m_PipelineState;

    // Resource created in an upload heap. Useful for drawing of dynamic geometry
    // or for uploading constant buffer data that changes every draw call.
    std::unique_ptr<UploadBuffer> m_UploadBuffer;

    // Resource state tracker is used by the command list to track (per command list)
    // the current state of a resource. The resource state tracker also tracks the
    // global state of a resource in order to minimize resource state transitions.
    std::unique_ptr<ResourceStateTracker> m_ResourceStateTracker;

    // The dynamic descriptor heap allows for descriptors to be staged before
    // being committed to the command list. Dynamic descriptors need to be
    // committed before a Draw or Dispatch.
    std::unique_ptr<DynamicDescriptorHeap> m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

    // Keep track of the currently bound descriptor heaps. Only change descriptor
    // heaps if they are different than the currently bound descriptor heaps.
    ID3D12DescriptorHeap* m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

    // Pipeline state object for Mip map generation.
    std::unique_ptr<GenerateMipsPSO> m_GenerateMipsPSO;
    // Pipeline state object for converting panorama (equirectangular) to cubemaps
    std::unique_ptr<PanoToCubemapPSO> m_PanoToCubemapPSO;

    using TrackedObjects = std::vector<Microsoft::WRL::ComPtr<ID3D12Object>>;

    // Objects that are being tracked by a command list that is "in-flight" on
    // the command-queue and cannot be deleted. To ensure objects are not deleted
    // until the command list is finished executing, a reference to the object
    // is stored. The referenced objects are released when the command list is
    // reset.
    TrackedObjects m_TrackedObjects;
};

// Definition for inline functions.
inline DirectX::XMVECTOR CommandList::GetCircleVector(size_t i, size_t tessellation) noexcept
{
    float angle = float(i) * DirectX::XM_2PI / float(tessellation);
    float dx, dz;

    DirectX::XMScalarSinCos(&dx, &dz, angle);

    DirectX::XMVECTORF32 v = { { { dx, 0, dz, 0 } } };
    return v;
}

inline DirectX::XMVECTOR CommandList::GetCircleTangent(size_t i, size_t tessellation) noexcept
{
    float angle = (float(i) * DirectX::XM_2PI / float(tessellation)) + DirectX::XM_PIDIV2;
    float dx, dz;

    DirectX::XMScalarSinCos(&dx, &dz, angle);

    DirectX::XMVECTORF32 v = { { { dx, 0, dz, 0 } } };
    return v;
}

inline void CommandList::ReverseWinding(IndexCollection& indices, VertexCollection& vertices)
{
    assert((indices.size() % 3) == 0);
    for (auto it = indices.begin(); it != indices.end(); it += 3)
    {
        std::swap(*it, *(it + 2));
    }

    for (auto it = vertices.begin(); it != vertices.end(); ++it)
    {
        it->TexCoord.x = (1.f - it->TexCoord.x);
    }
}

inline void CommandList::InvertNormals(VertexCollection& vertices)
{
    for (auto it = vertices.begin(); it != vertices.end(); ++it)
    {
        it->Normal.x = -it->Normal.x;
        it->Normal.y = -it->Normal.y;
        it->Normal.z = -it->Normal.z;
    }
}
