#include <Engine/Core/Model.h>
#include <Engine/Core/Events.h>

#include "Renderer.h"
#include <Engine/Core/RenderTarget.h>
#include <Engine/Core/VertexTypes.h>

class Scene;
class RootSignature;
class PipelineStateObject;
class CommandList;
class Device;
class StructuredBuffer;
struct MeshPrimitive;
class UnorderedAccessView;
struct ID3D12CommandSignature;

struct VisibilityStorageInfo
{
	u32 VertexOffset;
	u32 IndexOffset;
	u32 VertexCount;
	u32 IndexCount;
};

class VisibilityBufferRenderer : Renderer
{
public:
	bool Initialize();
	void BeginFrame();
	void RenderScene(RenderTarget& renderTarget, const Scene& scene, RefPtr<CommandList> commandList = nullptr);
	void EndFrame();

	virtual void RenderInstancedIndexedMesh(CommandList& commandList, const Mesh& mesh, const std::vector<XMMATRIX>& instances) override;
	
private:
	struct RasterizeTriangleStage
	{
		RefPtr<RootSignature> m_RootSignature;
		RefPtr<PipelineStateObject> m_PipelineState;
	} m_RasterizeTriangleState;

	struct DebugTriangleStage
	{
		RefPtr<RootSignature> m_RootSignature;
		RefPtr<PipelineStateObject> m_PipelineState;
	} m_DebugTriangleStage;

	struct MaterialCountStage
	{
		RefPtr<RootSignature> m_RootSignature;
		RefPtr<PipelineStateObject> m_PipelineState;
	} m_MaterialCountStage;

	struct MaterialPrefixSumStage
	{
		RefPtr<RootSignature> m_RootSignature;
		RefPtr<PipelineStateObject> m_PipelineState;
	} m_MaterialPrefixSumStage;

	struct PixelPositionBufferStage
	{
		RefPtr<RootSignature> m_RootSignature;
		RefPtr<PipelineStateObject> m_PipelineState;
	} m_PixelPositionBufferStage;

	struct MaterialResolveStage
	{
		RefPtr<RootSignature> m_RootSignature;
		RefPtr<PipelineStateObject> m_PipelineState;
	} m_MaterialResolveStage;

	RenderTarget m_VisibilityBuffer;
	RenderTarget m_GBuffer;

	RefPtr<StructuredBuffer> m_OffsetBuffer;
	RefPtr<UnorderedAccessView> m_OffsetBufferUAV;

	RefPtr<StructuredBuffer> m_MaterialCountBuffer;
	RefPtr<UnorderedAccessView> m_MaterialCountUAV;

	RefPtr<StructuredBuffer> m_MaterialOffsetBuffer;
	RefPtr<UnorderedAccessView> m_MaterialOffsetUAV;

	RefPtr<StructuredBuffer> m_PixelPositionBuffer;
	RefPtr<UnorderedAccessView> m_PixelPositionUAV;

	RefPtr<StructuredBuffer> m_IndirectComputeArguementBuffer;
	RefPtr<UnorderedAccessView> m_IndirectComputeArguementUAV;

	ComPtr<ID3D12CommandSignature> m_IndirectSignature;

	RefPtr<StructuredBuffer> m_VisibleVertexBuffer;
	RefPtr<StructuredBuffer> m_VisibleIndexBuffer;
	RefPtr<UnorderedAccessView> m_VisibleIndexBufferUAV;
	RefPtr<UnorderedAccessView> m_VisibleVertexBufferUAV;

	std::vector<VertexPositionNormalTangentBitangentTexture> m_VisibleVertexContainer;
	std::vector<u32> m_VisibleIndexContainer;

	std::unordered_map<MeshPrimitive*, VisibilityStorageInfo> m_StorageInfo;

	RefPtr<Device> m_Device;
};