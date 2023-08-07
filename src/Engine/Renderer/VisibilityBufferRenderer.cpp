#include <enginepch.h>
#include "VisibilityBufferRenderer.h"

#include <Engine/Core/Device.h>
#include <Engine/Core/Scene.h>
#include <Engine/Core/Application.h>
#include <Engine/Pipeline/CommandQueue.h>
#include <Engine/Pipeline/CommandList.h>
#include <Engine/Pipeline/RootSignature.h>
#include <Engine/Pipeline/PipelineStateObject.h>
#include <Engine/Buffers/Texture.h>
#include <Engine/Core/RenderTarget.h>
#include <Engine/Buffers/Texture.h>
#include <Engine/Core/Mesh.h>
#include <Engine/Buffers/StructuredBuffer.h>
#include <Engine/Buffers/UnorderedAccessView.h>
#include <Engine/Core/Material.h>
#include <Engine/Asset/MaterialAssetHandler.h>

#include <Engine/Buffers/IndexBuffer.h>
#include <Engine/Buffers/VertexBuffer.h>

constexpr u32 MaterialLimit = 96;

namespace RasterizeTriangleDataRootParameters
{
	enum
	{
		CameraCB,         // ConstantBuffer<CameraBuffer> MatCB : register(b0);
		InstancesSB,        // StructuredBuffer<matrix> InstancesSB : register( t0, space0 );
		DrawCallInfoCB,
		OffsetBuffer,
		NumParameters
	};
}
namespace DebugTriangleParameters
{
	enum
	{
		VisibilityBuffer,
		OffsetBuffer,
		GBuffer,
		NumParameters
	};
}
namespace MaterialCountParameters
{
	enum
	{
		VisibilityBuffer,
		OffsetBuffer,
		MaterialCountBuffer,
		NumParameters
	};
}
namespace MaterialPrefixSumParameters
{
	enum
	{
		MaterialCountBuffer,
		MaterialOffsetBuffer,
		IndirectArguementBuffer,
		NumParameters
	};
}
namespace PixelPositionBufferParameters
{
	enum
	{
		VisibilityBuffer,
		OffsetBuffer,
		MaterialCountBuffer,
		MaterialOffsetBuffer,
		PixelPositionBuffer,

		NumParameters
	};
}
namespace MaterialResolveParameters
{
	enum
	{
		VisibilityBuffer,
		OffsetBuffer,
		MaterialOffsetBuffer,
		MaterialCountBuffer,
		PixelPositionBuffer,
		RenderTarget,
		MaterialBufferOffsetConstant,
		VisibleVertexBuffer,
		VisibleIndexBuffer,
		AlbedoTexture,
		CameraCB,        
		InstancesSB,      
		NumParameters
	};
}

using namespace RasterizeTriangleDataRootParameters;

bool VisibilityBufferRenderer::Initialize()
{
	m_Device = Application::Get().GetDevice();

	auto& commandQueue = m_Device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto  commandList = commandQueue.GetCommandList();

	/*
		RASTERIZE TRIANGLE STAGE
	*/
	{
		ComPtr<ID3DBlob> vertexShaderBlob;
		ThrowIfFailed(D3DReadFileToBlob(L"Shaders/GenericMeshVS.cso", &vertexShaderBlob));

		ComPtr<ID3DBlob> pixelShaderBlob;
		ThrowIfFailed(D3DReadFileToBlob(L"Shaders/RasterizeTriangleDataPS.cso", &pixelShaderBlob));

		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
		CD3DX12_DESCRIPTOR_RANGE1 offsetBufferRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

		CD3DX12_ROOT_PARAMETER1 rootParameters[RasterizeTriangleDataRootParameters::NumParameters];
		rootParameters[RasterizeTriangleDataRootParameters::CameraCB].InitAsConstants((sizeof(Matrix) * 2) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		rootParameters[RasterizeTriangleDataRootParameters::InstancesSB].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
		rootParameters[RasterizeTriangleDataRootParameters::DrawCallInfoCB].InitAsConstants(4, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[RasterizeTriangleDataRootParameters::OffsetBuffer].InitAsDescriptorTable(1, &offsetBufferRange, D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
		CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler(0, D3D12_FILTER_ANISOTROPIC);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(RasterizeTriangleDataRootParameters::NumParameters, rootParameters, 1, &linearRepeatSampler,
			rootSignatureFlags);

		m_RasterizeTriangleState.m_RootSignature = m_Device->CreateRootSignature(rootSignatureDescription.Desc_1_1);

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
		//	CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC           SampleDesc;
		} pipelineStateStream;

		// Create a color buffer with sRGB for gamma correction.

		DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R32_UINT;
		DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;


		// Create an off-screen render target with a single color buffer and a depth buffer.
		auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(backBufferFormat, WND_PROP.Width, WND_PROP.Height, 1, 1, 1,
			0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		D3D12_CLEAR_VALUE colorClearValue;
		colorClearValue.Format = colorDesc.Format;
		colorClearValue.Color[0] = 0;

		auto colorTexture = m_Device->CreateTexture(colorDesc, &colorClearValue);
		colorTexture->SetName(L"Visibility Buffer");

		// Create a depth buffer.
		auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthBufferFormat, WND_PROP.Width, WND_PROP.Height, 1, 1, 1,
			0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
		D3D12_CLEAR_VALUE depthClearValue;
		depthClearValue.Format = depthDesc.Format;
		depthClearValue.DepthStencil = { 1.0f, 0 };

		auto depthTexture = m_Device->CreateTexture(depthDesc, &depthClearValue);
		depthTexture->SetName(L"Depth Render Target");

		// TODO: Actually bind default depth texture and dont create new one

		// Attach the textures to the render target.
		m_VisibilityBuffer.AttachTexture(AttachmentPoint::Color0, colorTexture);
		m_VisibilityBuffer.AttachTexture(AttachmentPoint::DepthStencil, depthTexture);

		constexpr u32 offsetStride = sizeof(u32) * 2;
		constexpr u32 numOffsets = 4096;

		m_OffsetBuffer = m_Device->CreateStructuredBuffer(numOffsets, offsetStride);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.Buffer.StructureByteStride = offsetStride;
		uavDesc.Buffer.CounterOffsetInBytes = 0;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = numOffsets;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		m_OffsetBufferUAV = m_Device->CreateUnorderedAccessView(m_OffsetBuffer, m_OffsetBuffer->GetCounterBuffer(), &uavDesc);

		colorTexture->CreateViews();

		D3D12_RT_FORMAT_ARRAY rtvFormats = {};
		rtvFormats.NumRenderTargets = 1;
		rtvFormats.RTFormats[0] = backBufferFormat;

		pipelineStateStream.pRootSignature = m_RasterizeTriangleState.m_RootSignature->GetD3D12RootSignature().Get();
		pipelineStateStream.InputLayout = VertexPositionNormalTangentBitangentTexture::InputLayout;
		pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
		pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
		pipelineStateStream.DSVFormat = depthBufferFormat;
		pipelineStateStream.RTVFormats = rtvFormats;
		//pipelineStateStream.SampleDesc = sampleDesc;

		m_RasterizeTriangleState.m_PipelineState = m_Device->CreatePipelineStateObject(pipelineStateStream);
	}

	/*
		MATERIAL COUNT STAGE
	*/
	{
		m_MaterialCountBuffer = m_Device->CreateStructuredBuffer(MaterialLimit, 4);
		m_MaterialOffsetBuffer = m_Device->CreateStructuredBuffer(MaterialLimit, 4);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.Buffer.StructureByteStride = 4;
		uavDesc.Buffer.CounterOffsetInBytes = 0;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = MaterialLimit;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		m_MaterialCountUAV = m_Device->CreateUnorderedAccessView(m_MaterialCountBuffer, m_MaterialCountBuffer->GetCounterBuffer(), &uavDesc);
		m_MaterialOffsetUAV = m_Device->CreateUnorderedAccessView(m_MaterialOffsetBuffer, m_MaterialOffsetBuffer->GetCounterBuffer(), &uavDesc);

		ComPtr<ID3DBlob> computeShaderBlob;
		ThrowIfFailed(D3DReadFileToBlob(L"Shaders/MaterialCountCS.cso", &computeShaderBlob));

		CD3DX12_DESCRIPTOR_RANGE1 visbufferRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
		CD3DX12_DESCRIPTOR_RANGE1 offsetBufferRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);
		CD3DX12_DESCRIPTOR_RANGE1 bufferRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2, 0,
			D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

		CD3DX12_ROOT_PARAMETER1 rootParameters[MaterialCountParameters::NumParameters];
		rootParameters[MaterialCountParameters::VisibilityBuffer].InitAsDescriptorTable(1, &visbufferRange, D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[MaterialCountParameters::OffsetBuffer].InitAsDescriptorTable(1, &offsetBufferRange, D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[MaterialCountParameters::MaterialCountBuffer].InitAsDescriptorTable(1, &bufferRange, D3D12_SHADER_VISIBILITY_ALL);

		CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(MaterialCountParameters::NumParameters, rootParameters, 1, &linearRepeatSampler);

		m_MaterialCountStage.m_RootSignature = m_Device->CreateRootSignature(rootSignatureDescription.Desc_1_1);

		struct PipelineStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE        pRootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_CS                    CS;
		} pipelineStateStream;

		pipelineStateStream.pRootSignature = m_MaterialCountStage.m_RootSignature->GetD3D12RootSignature().Get();
		pipelineStateStream.CS = CD3DX12_SHADER_BYTECODE(computeShaderBlob.Get());

		m_MaterialCountStage.m_PipelineState = m_Device->CreatePipelineStateObject(pipelineStateStream);
	}

	/*
		MATERIAL PREFIX SUM STAGE
	*/
	{
		ComPtr<ID3DBlob> computeShaderBlob;
		ThrowIfFailed(D3DReadFileToBlob(L"Shaders/MaterialPrefixSumCS.cso", &computeShaderBlob));

		CD3DX12_DESCRIPTOR_RANGE1 countBufferRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0,
			D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
		CD3DX12_DESCRIPTOR_RANGE1 offsetBufferRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, 0,
			D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

		CD3DX12_DESCRIPTOR_RANGE1 indirectBufferRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

		CD3DX12_ROOT_PARAMETER1 rootParameters[MaterialPrefixSumParameters::NumParameters];
		rootParameters[MaterialPrefixSumParameters::MaterialCountBuffer].InitAsDescriptorTable(1, &countBufferRange, D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[MaterialPrefixSumParameters::MaterialOffsetBuffer].InitAsDescriptorTable(1, &offsetBufferRange, D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[MaterialPrefixSumParameters::IndirectArguementBuffer].InitAsDescriptorTable(1, &indirectBufferRange, D3D12_SHADER_VISIBILITY_ALL);

		CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription(MaterialPrefixSumParameters::NumParameters, rootParameters, 1, &linearRepeatSampler);

		m_MaterialPrefixSumStage.m_RootSignature = m_Device->CreateRootSignature(rootSignatureDescription.Desc_1_1);
		m_MaterialPrefixSumStage.m_RootSignature->GetD3D12RootSignature()->SetName(L"MaterialPrefixSumStage");

		// Setup the pipeline state.
		struct PipelineStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE        pRootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_CS                    CS;
		} pipelineStateStream;

		pipelineStateStream.pRootSignature = m_MaterialPrefixSumStage.m_RootSignature->GetD3D12RootSignature().Get();
		pipelineStateStream.CS = CD3DX12_SHADER_BYTECODE(computeShaderBlob.Get());

		m_MaterialPrefixSumStage.m_PipelineState = m_Device->CreatePipelineStateObject(pipelineStateStream);
	}

	/*
		PIXEL POSITION BUFFER STAGE
	*/
	{
		ComPtr<ID3DBlob> computeShaderBlob;
		ThrowIfFailed(D3DReadFileToBlob(L"Shaders/PixelPositionBufferCS.cso", &computeShaderBlob));

		CD3DX12_DESCRIPTOR_RANGE1 visBufferRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
		CD3DX12_DESCRIPTOR_RANGE1 offsetBufferRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);
		CD3DX12_DESCRIPTOR_RANGE1 countBufferRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2);
		CD3DX12_DESCRIPTOR_RANGE1 materialOffsetBufferRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 3);
		CD3DX12_DESCRIPTOR_RANGE1 pixelPositionBufferRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 4);

		CD3DX12_ROOT_PARAMETER1 rootParameters[PixelPositionBufferParameters::NumParameters];
		rootParameters[PixelPositionBufferParameters::VisibilityBuffer].InitAsDescriptorTable(1, &visBufferRange, D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[PixelPositionBufferParameters::OffsetBuffer].InitAsDescriptorTable(1, &offsetBufferRange, D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[PixelPositionBufferParameters::MaterialCountBuffer].InitAsDescriptorTable(1, &countBufferRange, D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[PixelPositionBufferParameters::MaterialOffsetBuffer].InitAsDescriptorTable(1, &materialOffsetBufferRange, D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[PixelPositionBufferParameters::PixelPositionBuffer].InitAsDescriptorTable(1, &pixelPositionBufferRange, D3D12_SHADER_VISIBILITY_ALL);

		CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(PixelPositionBufferParameters::NumParameters, rootParameters, 1, &linearRepeatSampler);

		m_PixelPositionBufferStage.m_RootSignature = m_Device->CreateRootSignature(rootSignatureDescription.Desc_1_1);
		m_PixelPositionBufferStage.m_RootSignature->GetD3D12RootSignature()->SetName(L"PixelPositionBufferStage");
		// Setup the pipeline state.
		struct PipelineStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE        pRootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_CS                    CS;
		} pipelineStateStream;

		pipelineStateStream.pRootSignature = m_PixelPositionBufferStage.m_RootSignature->GetD3D12RootSignature().Get();
		pipelineStateStream.CS = CD3DX12_SHADER_BYTECODE(computeShaderBlob.Get());

		m_PixelPositionBufferStage.m_PipelineState = m_Device->CreatePipelineStateObject(pipelineStateStream);
	}


	/*
		MATERIAL RESOLVE STAGE
	*/
	{
		ComPtr<ID3DBlob> computeShaderBlob;
		ThrowIfFailed(D3DReadFileToBlob(L"Shaders/MaterialResolveCS.cso", &computeShaderBlob));

		CD3DX12_DESCRIPTOR_RANGE1 visBufferRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
		CD3DX12_DESCRIPTOR_RANGE1 offsetBufferRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);
		CD3DX12_DESCRIPTOR_RANGE1 matOffsetBufferRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2);
		CD3DX12_DESCRIPTOR_RANGE1 matCountBufferRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 3);
		CD3DX12_DESCRIPTOR_RANGE1 ppBufferRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 4, 0);
		CD3DX12_DESCRIPTOR_RANGE1 renderTargetRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 5, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
		CD3DX12_DESCRIPTOR_RANGE1 vbBufferRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 7, 0);
		CD3DX12_DESCRIPTOR_RANGE1 ibBufferRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 8, 0);
		CD3DX12_DESCRIPTOR_RANGE1 albedoTextureRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MaterialLimit, 1);

		CD3DX12_ROOT_PARAMETER1 rootParameters[MaterialResolveParameters::NumParameters];
		rootParameters[MaterialResolveParameters::VisibilityBuffer].InitAsDescriptorTable(1, &visBufferRange, D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[MaterialResolveParameters::OffsetBuffer].InitAsDescriptorTable(1, &offsetBufferRange, D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[MaterialResolveParameters::MaterialOffsetBuffer].InitAsDescriptorTable(1, &matOffsetBufferRange, D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[MaterialResolveParameters::MaterialCountBuffer].InitAsDescriptorTable(1, &matCountBufferRange, D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[MaterialResolveParameters::PixelPositionBuffer].InitAsDescriptorTable(1, &ppBufferRange, D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[MaterialResolveParameters::RenderTarget].InitAsDescriptorTable(1, &renderTargetRange, D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[MaterialResolveParameters::MaterialBufferOffsetConstant].InitAsConstants(2, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[MaterialResolveParameters::VisibleVertexBuffer].InitAsDescriptorTable(1, &vbBufferRange, D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[MaterialResolveParameters::VisibleIndexBuffer].InitAsDescriptorTable(1, &ibBufferRange, D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[MaterialResolveParameters::AlbedoTexture].InitAsDescriptorTable(1, &albedoTextureRange, D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[MaterialResolveParameters::CameraCB].InitAsConstants(192 / 4, 1, 0, D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[MaterialResolveParameters::InstancesSB].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
		//(sizeof(Matrix) * 3) / 4
		CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription(MaterialResolveParameters::NumParameters, rootParameters, 1, &linearRepeatSampler);
	
		m_MaterialResolveStage.m_RootSignature = m_Device->CreateRootSignature(rootSignatureDescription.Desc_1_1);
		m_MaterialResolveStage.m_RootSignature->GetD3D12RootSignature()->SetName(L"MaterialResolve");

		// Setup the pipeline state.
		struct PipelineStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE        pRootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_CS                    CS;
		} pipelineStateStream;

		pipelineStateStream.pRootSignature = m_MaterialResolveStage.m_RootSignature->GetD3D12RootSignature().Get();
		pipelineStateStream.CS = CD3DX12_SHADER_BYTECODE(computeShaderBlob.Get());

		m_MaterialResolveStage.m_PipelineState = m_Device->CreatePipelineStateObject(pipelineStateStream);

		D3D12_INDIRECT_ARGUMENT_DESC args[2];
		args[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
		args[0].Constant.DestOffsetIn32BitValues = 0;
		args[0].Constant.Num32BitValuesToSet = 2;
		args[0].Constant.RootParameterIndex = MaterialResolveParameters::MaterialBufferOffsetConstant;
		args[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

		D3D12_COMMAND_SIGNATURE_DESC signatureDesc;
		signatureDesc.ByteStride = sizeof(D3D12_DISPATCH_ARGUMENTS) + sizeof(UINT) * 2;
		signatureDesc.NumArgumentDescs = 2;
		signatureDesc.pArgumentDescs = args;
		signatureDesc.NodeMask = 1;

		ThrowIfFailed(m_Device->GetD3D12Device()->CreateCommandSignature(&signatureDesc, pipelineStateStream.pRootSignature, IID_PPV_ARGS(&m_IndirectSignature)));
	}

	/*
		FULLSCREEN DEBUG STAGE
	*/
	{
		ComPtr<ID3DBlob> vertexShaderBlob;
		ThrowIfFailed(D3DReadFileToBlob(L"Shaders/FullscreenVS.cso", &vertexShaderBlob));

		ComPtr<ID3DBlob> pixelShaderBlob;
		ThrowIfFailed(D3DReadFileToBlob(L"Shaders/DeferredLitPS.cso", &pixelShaderBlob));

		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		CD3DX12_DESCRIPTOR_RANGE1 vbufferRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
		CD3DX12_DESCRIPTOR_RANGE1 obufferRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);
		CD3DX12_DESCRIPTOR_RANGE1 gbufferRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

		CD3DX12_ROOT_PARAMETER1 rootParameters[DebugTriangleParameters::NumParameters];
		rootParameters[DebugTriangleParameters::VisibilityBuffer].InitAsDescriptorTable(1, &vbufferRange, D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[DebugTriangleParameters::OffsetBuffer].InitAsDescriptorTable(1, &obufferRange, D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[DebugTriangleParameters::GBuffer].InitAsDescriptorTable(1, &gbufferRange, D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
		CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler(0, D3D12_FILTER_ANISOTROPIC);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(DebugTriangleParameters::NumParameters, rootParameters, 1, &linearRepeatSampler,
			rootSignatureFlags);

		m_DebugTriangleStage.m_RootSignature = m_Device->CreateRootSignature(rootSignatureDescription.Desc_1_1);

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

		pipelineStateStream.pRootSignature = m_DebugTriangleStage.m_RootSignature->GetD3D12RootSignature().Get();
		pipelineStateStream.InputLayout = VertexPositionNormalTangentBitangentTexture::InputLayout;
		pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
		pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
		pipelineStateStream.DSVFormat = depthBufferFormat;
		pipelineStateStream.RTVFormats = rtvFormats;
		pipelineStateStream.SampleDesc = sampleDesc;

		m_DebugTriangleStage.m_PipelineState = m_Device->CreatePipelineStateObject(pipelineStateStream);
	}

	{
		DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R32_FLOAT;
		DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

		// Create an off-screen render target with a single color buffer and a depth buffer.
		auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(backBufferFormat, WND_PROP.Width * 4, WND_PROP.Height, 1, 1, 1,
			0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		D3D12_CLEAR_VALUE colorClearValue;
		colorClearValue.Format = colorDesc.Format;
		colorClearValue.Color[0] = 0;

		auto colorTexture = m_Device->CreateTexture(colorDesc, &colorClearValue);
		colorTexture->SetName(L"GBuffer Texture");


		// Create a depth buffer.
		//auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthBufferFormat, WND_PROP.Width * 4, WND_PROP.Height, 1, 1, 1,
		//	0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
		//D3D12_CLEAR_VALUE depthClearValue;
		//depthClearValue.Format = depthDesc.Format;
		//depthClearValue.DepthStencil = { 1.0f, 0 };

		//auto depthTexture = m_Device->CreateTexture(depthDesc, &depthClearValue);
		//depthTexture->SetName(L"Depth Render Target");

		// TODO: Actually bind default depth texture and dont create new one

		// Attach the textures to the render target.
		m_GBuffer.AttachTexture(AttachmentPoint::Color0, colorTexture);
		//m_GBuffer.AttachTexture(AttachmentPoint::DepthStencil, depthTexture);

		colorTexture->CreateViews();
	}

	commandQueue.Flush();  // Wait for loading operations to complete before rendering the first frame.

	return true;
}

void VisibilityBufferRenderer::BeginFrame()
{
}

static u32 drawCallId = 0;

void VisibilityBufferRenderer::RenderScene(RenderTarget& renderTarget, const Scene& scene, RefPtr<CommandList> commandList)
{
	if (!m_PixelPositionUAV || !m_PixelPositionBuffer || (u64)WND_PROP.Height * WND_PROP.Width != m_PixelPositionBuffer->GetNumElements())
	{
		//auto& copyQueue = m_Device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
		//auto list = copyQueue.GetCommandList();

		u32 num = WND_PROP.Height * WND_PROP.Width;

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.Buffer.StructureByteStride = sizeof(Vector2);
		uavDesc.Buffer.CounterOffsetInBytes = 0;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = num;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		m_PixelPositionBuffer = m_Device->CreateStructuredBuffer(num, sizeof(Vector2));
		m_PixelPositionUAV = m_Device->CreateUnorderedAccessView(m_PixelPositionBuffer, m_PixelPositionBuffer->GetCounterBuffer(), &uavDesc);
		
		//copyQueue.WaitForFenceValue(copyQueue.ExecuteCommandList(list));
		//copyQueue.Flush();
	}

	auto& commandQueue = m_Device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto& computeQueue = m_Device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE);

	if (!commandList)
	{
		commandList = commandQueue.GetCommandList();
	}

	u32 materialCount = 0;
	std::unordered_map<Material*, u32> materialCache;


	/*
		RASTERIZE TRIANGLE STAGE
	*/
	{
		m_VisibilityBuffer.Clear(commandList, { 0, 0, 0, 0 }, true);

		commandList->SetPipelineState(m_RasterizeTriangleState.m_PipelineState);
		commandList->SetGraphicsRootSignature(m_RasterizeTriangleState.m_RootSignature);

		commandList->SetViewport(WND_PROP.Viewport);
		commandList->SetScissorRect(WND_PROP.ScissorRect);
		commandList->SetRenderTarget(m_VisibilityBuffer);
		commandList->SetUnorderedAccessView(RasterizeTriangleDataRootParameters::OffsetBuffer, 0, m_OffsetBufferUAV, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		XMMATRIX matrices[2] = {
			scene.GetCameraRef().get_ViewMatrix(),
			scene.GetCameraRef().get_ProjectionMatrix()
		};

		commandList->SetGraphics32BitConstants(RasterizeTriangleDataRootParameters::CameraCB, matrices);

		drawCallId = 0;
		for (RefPtr<Model> model : scene.GetModels())
		{
			for (auto& material : model->GetMesh()->GetMaterials())
			{
				if (materialCache.find(material.get()) == materialCache.end())
				{
					materialCache.emplace(std::make_pair(material.get(), materialCount));
					materialCount++;
				}
			}
		
			for (auto& primitive : model->GetMesh()->GetPrimitives())
			{
				if (m_StorageInfo.find((MeshPrimitive*)&primitive) == m_StorageInfo.end())
				{
					auto vertexContainerOffset = m_VisibleVertexContainer.size();
					auto indexContainerOffset = m_VisibleIndexContainer.size();
					m_VisibleVertexContainer.insert(m_VisibleVertexContainer.end(), primitive.m_VertexContainer.begin(), primitive.m_VertexContainer.end());
					m_VisibleIndexContainer.insert(m_VisibleIndexContainer.end(), primitive.m_IndexContainer.begin(), primitive.m_IndexContainer.end());
					m_StorageInfo.emplace(std::make_pair((MeshPrimitive*)&primitive, VisibilityStorageInfo{
						(u32)(vertexContainerOffset),
						(u32)(indexContainerOffset),
						(u32)(m_VisibleVertexContainer.size() - vertexContainerOffset),
						(u32)(m_VisibleIndexContainer.size() - indexContainerOffset)
					}));
				}
			}
		}
		for (RefPtr<Model> model : scene.GetModels())
		{
			XMMATRIX worldMatrix = model->GetTransform() * model->GetRotation() * model->GetScale();

			std::vector<XMMATRIX> instances;
			instances.push_back(worldMatrix);
			RenderInstancedIndexedMesh(*commandList, *model->GetMesh(), instances);
		}
		commandList->TransitionBarrier(m_VisibilityBuffer.GetTexture(AttachmentPoint::Color0), D3D12_RESOURCE_STATE_COMMON);

		commandQueue.WaitForFenceValue(commandQueue.ExecuteCommandList(commandList));
		if (!m_IndirectComputeArguementBuffer || !m_IndirectComputeArguementUAV || m_IndirectComputeArguementBuffer->GetNumElements() != materialCount)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
			uavDesc.Format = DXGI_FORMAT_UNKNOWN;
			uavDesc.Buffer.StructureByteStride = sizeof(D3D12_DISPATCH_ARGUMENTS) + sizeof(UINT) * 2;
			uavDesc.Buffer.CounterOffsetInBytes = 0;
			uavDesc.Buffer.FirstElement = 0;
			uavDesc.Buffer.NumElements = materialCount;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

			m_IndirectComputeArguementBuffer = m_Device->CreateStructuredBuffer(materialCount, sizeof(D3D12_DISPATCH_ARGUMENTS) + sizeof(UINT) * 2);
			m_IndirectComputeArguementUAV = m_Device->CreateUnorderedAccessView(m_IndirectComputeArguementBuffer, m_IndirectComputeArguementBuffer->GetCounterBuffer(), &uavDesc);
		}

		if (!m_VisibleIndexBuffer || m_VisibleIndexBuffer->GetNumElements() != m_VisibleIndexContainer.size())
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
			uavDesc.Format = DXGI_FORMAT_UNKNOWN;
			uavDesc.Buffer.StructureByteStride = sizeof(u32);
			uavDesc.Buffer.CounterOffsetInBytes = 0;
			uavDesc.Buffer.FirstElement = 0;
			uavDesc.Buffer.NumElements = m_VisibleIndexContainer.size();
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

			m_VisibleIndexBuffer = commandList->CopyStructuredBuffer(m_VisibleIndexContainer);
			m_VisibleIndexBufferUAV = m_Device->CreateUnorderedAccessView(m_VisibleIndexBuffer, m_VisibleIndexBuffer->GetCounterBuffer(), &uavDesc);
		}
		if (!m_VisibleVertexBuffer || m_VisibleVertexBuffer->GetNumElements() != m_VisibleVertexContainer.size())
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
			uavDesc.Format = DXGI_FORMAT_UNKNOWN;
			uavDesc.Buffer.StructureByteStride = sizeof(VertexPositionNormalTangentBitangentTexture);
			uavDesc.Buffer.CounterOffsetInBytes = 0;
			uavDesc.Buffer.FirstElement = 0;
			uavDesc.Buffer.NumElements = m_VisibleVertexContainer.size();
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

			m_VisibleVertexBuffer = commandList->CopyStructuredBuffer(m_VisibleVertexContainer);
			m_VisibleVertexBufferUAV = m_Device->CreateUnorderedAccessView(m_VisibleVertexBuffer, m_VisibleVertexBuffer->GetCounterBuffer(), &uavDesc);
		}
	}

	/*
		MATERIAL COUNT STAGE
	*/
	{
		auto computeList = computeQueue.GetCommandList();

		computeList->SetPipelineState(m_MaterialCountStage.m_PipelineState);
		computeList->SetComputeRootSignature(m_MaterialCountStage.m_RootSignature);

		auto visbuffer = m_VisibilityBuffer.GetTexture(AttachmentPoint::Color0);

		computeList->SetUnorderedAccessView(MaterialCountParameters::VisibilityBuffer, 0, visbuffer, 0, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeList->SetUnorderedAccessView(MaterialCountParameters::OffsetBuffer, 0, m_OffsetBufferUAV, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeList->SetUnorderedAccessView(MaterialCountParameters::MaterialCountBuffer, 0, m_MaterialCountUAV, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		computeList->Dispatch((u32)std::ceil(m_VisibilityBuffer.GetWidth() / 16) + 1, (u32)std::ceil(m_VisibilityBuffer.GetHeight() / 16) + 1);

		computeQueue.WaitForFenceValue(computeQueue.ExecuteCommandList(computeList));
	}

	/*
		MATERIAL PREFIX SUM SCAN STAGE
	*/
	{
		auto computeList = computeQueue.GetCommandList();

		computeList->SetComputeRootSignature(m_MaterialPrefixSumStage.m_RootSignature);
		computeList->SetPipelineState(m_MaterialPrefixSumStage.m_PipelineState);

		computeList->SetUnorderedAccessView(MaterialPrefixSumParameters::MaterialCountBuffer, 0, m_MaterialCountUAV, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeList->SetUnorderedAccessView(MaterialPrefixSumParameters::MaterialOffsetBuffer, 0, m_MaterialOffsetUAV, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeList->SetUnorderedAccessView(MaterialPrefixSumParameters::IndirectArguementBuffer, 0, m_IndirectComputeArguementUAV, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		computeList->TransitionBarrier(m_MaterialCountBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeList->TransitionBarrier(m_MaterialOffsetBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeList->TransitionBarrier(m_IndirectComputeArguementBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		computeList->Dispatch(1);

		computeList->UAVBarrier(m_MaterialCountUAV->GetResource());
		computeList->UAVBarrier(m_MaterialOffsetUAV->GetResource());
		computeList->UAVBarrier(m_IndirectComputeArguementUAV->GetResource());

		computeQueue.WaitForFenceValue(computeQueue.ExecuteCommandList(computeList));
	}

	/*
		PIXEL XY BUFFER STAGE
	*/
	{
		auto computeList = computeQueue.GetCommandList();

		computeList->SetPipelineState(m_PixelPositionBufferStage.m_PipelineState);
		computeList->SetComputeRootSignature(m_PixelPositionBufferStage.m_RootSignature);

		auto visbuffer = m_VisibilityBuffer.GetTexture(AttachmentPoint::Color0);

		computeList->SetUnorderedAccessView(PixelPositionBufferParameters::VisibilityBuffer, 0, visbuffer, 0, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeList->SetUnorderedAccessView(PixelPositionBufferParameters::OffsetBuffer, 0, m_OffsetBufferUAV, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeList->SetUnorderedAccessView(PixelPositionBufferParameters::MaterialCountBuffer, 0, m_MaterialCountUAV, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeList->SetUnorderedAccessView(PixelPositionBufferParameters::MaterialOffsetBuffer, 0, m_MaterialOffsetUAV, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeList->SetUnorderedAccessView(PixelPositionBufferParameters::PixelPositionBuffer, 0, m_PixelPositionUAV, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		computeList->Dispatch((u32)std::ceil(m_VisibilityBuffer.GetWidth() / 4) + 1, (u32)std::ceil(m_VisibilityBuffer.GetHeight() / 8) + 1);

		computeList->UAVBarrier(m_MaterialCountUAV->GetResource());
		computeList->UAVBarrier(m_MaterialOffsetUAV->GetResource());
		computeList->UAVBarrier(m_PixelPositionUAV->GetResource());

		computeQueue.WaitForFenceValue(computeQueue.ExecuteCommandList(computeList));
	}

	/*
		MATERIAL RESOLVE STAGE
	*/
	{
		auto computeList = computeQueue.GetCommandList();

		computeList->SetComputeRootSignature(m_MaterialResolveStage.m_RootSignature);
		computeList->SetPipelineState(m_MaterialResolveStage.m_PipelineState);

		auto visbuffer = m_VisibilityBuffer.GetTexture(AttachmentPoint::Color0);
		auto target = m_GBuffer.GetTexture(AttachmentPoint::Color0);

		computeList->SetUnorderedAccessView(MaterialResolveParameters::VisibilityBuffer, 0, visbuffer, 0, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeList->SetUnorderedAccessView(MaterialResolveParameters::OffsetBuffer, 0, m_OffsetBufferUAV, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeList->SetUnorderedAccessView(MaterialResolveParameters::MaterialOffsetBuffer, 0, m_MaterialOffsetUAV, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeList->SetUnorderedAccessView(MaterialResolveParameters::MaterialCountBuffer, 0, m_MaterialCountUAV, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeList->SetUnorderedAccessView(MaterialResolveParameters::PixelPositionBuffer, 0, m_PixelPositionUAV, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeList->SetUnorderedAccessView(MaterialResolveParameters::RenderTarget, 0, target, 0, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		computeList->SetUnorderedAccessView(MaterialResolveParameters::VisibleVertexBuffer, 0, m_VisibleVertexBufferUAV, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeList->SetUnorderedAccessView(MaterialResolveParameters::VisibleIndexBuffer, 0, m_VisibleIndexBufferUAV, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		struct CameraCB
		{
			XMMATRIX view;
			XMMATRIX projection;
			XMMATRIX inverseviewproj;
		} cameraCB;
		
		cameraCB.view = scene.GetCameraRef().get_ViewMatrix();
		cameraCB.projection = scene.GetCameraRef().get_ProjectionMatrix();
		cameraCB.inverseviewproj = scene.GetCameraRef().get_InverseViewProjectionMatrix();

		computeList->SetCompute32BitConstants(MaterialResolveParameters::CameraCB, cameraCB);

		// TODO: Make instance array global and access individual instance with drawcall_id
		for (RefPtr<Model> model : scene.GetModels())
		{
			XMMATRIX worldMatrix = model->GetTransform() * model->GetRotation() * model->GetScale();
			std::vector<XMMATRIX> instances;
			instances.push_back(worldMatrix);
			computeList->SetComputeDynamicStructuredBuffer(MaterialResolveParameters::InstancesSB, instances);
			break;
		}

		for (auto& mat : materialCache)
		{
			computeList->SetShaderResourceView(MaterialResolveParameters::AlbedoTexture, mat.second, mat.first->GetTexture(Material::TextureType::Diffuse), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		}
		if (materialCount < MaterialLimit)
		{
			auto defaultMaterial = MaterialAssetHandler::Get().GetDefaultMaterial()->GetTexture(Material::TextureType::Diffuse);
			for (u32 i = materialCount; i < MaterialLimit; ++i)
			{
				computeList->SetShaderResourceView(MaterialResolveParameters::AlbedoTexture, i, defaultMaterial, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			}
		}

		computeList->FlushResourceBarriers();
		
		computeList->CommitStagedDescriptorsForDispatch();

		computeList->GetD3D12CommandList()->ExecuteIndirect(m_IndirectSignature.Get(), materialCount, m_IndirectComputeArguementBuffer->GetD3D12Resource().Get(), 0, NULL, 0);

		//computeList->Dispatch(1024);

		//computeList->UAVBarrier(target);


		computeQueue.WaitForFenceValue(computeQueue.ExecuteCommandList(computeList));
	}

	/*
		DEBUG TRIANGLE STAGE
	*/
	if (true)
	{
		commandList = commandQueue.GetCommandList();

		commandList->SetRenderTarget(renderTarget);
		commandList->SetPipelineState(m_DebugTriangleStage.m_PipelineState);
		commandList->SetGraphicsRootSignature(m_DebugTriangleStage.m_RootSignature);

		commandList->SetViewport(WND_PROP.Viewport);
		commandList->SetScissorRect(WND_PROP.ScissorRect);
		commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		auto visbuffer = m_VisibilityBuffer.GetTexture(AttachmentPoint::Color0);
		auto gbuffer = m_GBuffer.GetTexture(AttachmentPoint::Color0);

		commandList->SetUnorderedAccessView(DebugTriangleParameters::VisibilityBuffer, 0, visbuffer, 0, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->SetUnorderedAccessView(DebugTriangleParameters::OffsetBuffer, 0, m_OffsetBufferUAV, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->SetShaderResourceView(DebugTriangleParameters::GBuffer, 0, gbuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	
		commandList->Draw(3, 1, 0, 0);

		commandList->TransitionBarrier(gbuffer, D3D12_RESOURCE_STATE_COMMON);

		commandQueue.WaitForFenceValue(commandQueue.ExecuteCommandList(commandList));
	}
}

void VisibilityBufferRenderer::EndFrame()
{
}

void VisibilityBufferRenderer::RenderInstancedIndexedMesh(CommandList& commandList, const Mesh& mesh, const std::vector<XMMATRIX>& instances)
{
	commandList.SetPrimitiveTopology(mesh.GetPrimitiveTopology());
	commandList.SetGraphicsDynamicStructuredBuffer(RasterizeTriangleDataRootParameters::InstancesSB, instances);

	auto& primitives = mesh.GetPrimitives();
	for (size_t i = 0; i < primitives.size(); ++i)
	{
		auto& primitive = primitives[i];
		
		struct PackedDrawCallInfo
		{
			u32 DrawCallId;
			u32 MaterialId;
			u32 MeshVertexOffset;
			u32 MeshIndexOffset;
		} drawCallInfo;
		
		drawCallInfo.DrawCallId = drawCallId;
		drawCallInfo.MaterialId = primitive.m_MaterialIndex;
		drawCallId++;

		if (auto iterator = m_StorageInfo.find((MeshPrimitive*)&primitive); iterator != m_StorageInfo.end())
		{
			drawCallInfo.MeshIndexOffset = iterator->second.IndexOffset;
			drawCallInfo.MeshVertexOffset = iterator->second.VertexOffset;
		}

		commandList.SetGraphics32BitConstants(RasterizeTriangleDataRootParameters::DrawCallInfoCB, drawCallInfo);

		commandList.SetVertexBuffer(0, primitive.m_VertexBuffer);

		if (auto indexCount = primitive.m_IndexBuffer->GetNumIndices(); indexCount > 0)
		{
			commandList.SetIndexBuffer(primitive.m_IndexBuffer);
			commandList.DrawIndexed(indexCount, instances.size());
		}
		else if (auto vertexCount = primitive.m_VertexBuffer->GetNumVertices(); vertexCount > 0)
		{
			commandList.Draw(vertexCount, instances.size());
		}
	}
}
