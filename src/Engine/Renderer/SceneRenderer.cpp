#include <enginepch.h>
#include "SceneRenderer.h"

#include <Engine/Core/Device.h>
#include <Engine/Core/Scene.h>
#include <Engine/Core/Application.h>
#include <Engine/Pipeline/CommandQueue.h>
#include <Engine/Pipeline/CommandList.h>
#include <Engine/Pipeline/RootSignature.h>
#include <Engine/Pipeline/PipelineStateObject.h>
#include <Engine/Buffers/Texture.h>
#include <Engine/Core/RenderTarget.h>
#include <Engine/Core/Mesh.h>

bool SceneRenderer::Initialize()
{
	m_Device = Application::Get().GetDevice();

	auto& commandQueue = m_Device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto  commandList = commandQueue.GetCommandList();

	//commandQueue.ExecuteCommandList(commandList);

	// Load the vertex shader.
	auto vertexShaderBlob = DX::ReadData(L"Shaders/VertexShaderVS.cso");

	// Load the pixel shader.
	auto pixelShaderBlob = DX::ReadData(L"Shaders/PixelShaderPS.cso");

	// Create a root signature.
	// Allow input layout and deny unnecessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);

	CD3DX12_ROOT_PARAMETER1 rootParameters[GenericRootParameters::NumGenericRootParameters];
	rootParameters[GenericRootParameters::CameraCB].InitAsConstants((sizeof(Matrix) * 2) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[GenericRootParameters::MaterialCB].InitAsConstantBufferView(0, 1, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
	//rootParameters[RootParameters::LightPropertiesCB].InitAsConstants(sizeof(LightProperties) / 4, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[GenericRootParameters::InstancesSB].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
	//rootParameters[RootParameters::PointLights].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
	//rootParameters[RootParameters::SpotLights].InitAsShaderResourceView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[GenericRootParameters::Textures].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
	CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler(0, D3D12_FILTER_ANISOTROPIC);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Init_1_1(GenericRootParameters::NumGenericRootParameters, rootParameters, 1, &linearRepeatSampler,
		rootSignatureFlags);

	m_RootSignature = m_Device->CreateRootSignature(rootSignatureDescription.Desc_1_1);

	// Setup the pipeline state.
	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE        pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT          InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY    PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_VS                    VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS                    PS;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT  DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
		CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC           SampleDesc;
	} pipelineStateStream;

	// Create a color buffer with sRGB for gamma correction.
	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

	// Check the best multisample quality level that can be used for the given back buffer format.
	DXGI_SAMPLE_DESC sampleDesc = m_Device->GetMultisampleQualityLevels(backBufferFormat);

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = backBufferFormat;

	pipelineStateStream.pRootSignature = m_RootSignature->GetD3D12RootSignature().Get();
	pipelineStateStream.InputLayout = VertexPositionNormalTangentBitangentTexture::InputLayout;
	pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE{ vertexShaderBlob.data(), vertexShaderBlob.size() };
	pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE{ pixelShaderBlob.data(), pixelShaderBlob.size() };
	pipelineStateStream.DSVFormat = depthBufferFormat;
	pipelineStateStream.RTVFormats = rtvFormats;
	pipelineStateStream.SampleDesc = sampleDesc;

	//m_PipelineState = m_Device->CreatePipelineStateObject(pipelineStateStream);

	commandQueue.Flush();  // Wait for loading operations to complete before rendering the first frame.

	m_VisibilityBufferRenderer = MakeUnique<VisibilityBufferRenderer>();
	m_VisibilityBufferRenderer->Initialize();

	return true;
}

void SceneRenderer::RenderScene(RenderTarget& renderTarget, const Scene& scene, RefPtr<CommandList> commandList)
{
	m_VisibilityBufferRenderer->RenderScene(renderTarget, scene);


	//for (RefPtr<Model> model : scene.m_Models)
	//{
	//	XMMATRIX worldMatrix = model->GetTransform() * model->GetRotation() * model->GetScale();

	//	std::vector<XMMATRIX> instances;
	//	instances.push_back(worldMatrix);
	//	RenderInstancedIndexedMesh(*commandList, *model->GetMesh(), instances);
	//}
}
