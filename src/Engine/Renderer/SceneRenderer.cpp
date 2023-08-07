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
