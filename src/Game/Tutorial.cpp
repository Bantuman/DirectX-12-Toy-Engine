#include "gamepch.h"
#include "Tutorial.h"

#include <Engine/Core/Application.h>
#include <Engine/Core/Window.h>
#include <Engine/Core/Helpers.h>

#include <Engine/Buffers/UploadBuffer.h>
#include <Engine/Buffers/VertexBuffer.h>

#include <Engine/Pipeline/CommandList.h>
#include <Engine/Pipeline/CommandQueue.h>
#include <Engine/Core/Device.h>
#include <Engine/Core/GUI.h>
#include <Engine/Core/Helpers.h>
#include <Engine/Core/Material.h>
#include <Engine/Core/Mesh.h>
#include <Engine/Pipeline/PipelineStateObject.h>
#include <Engine/Pipeline/RootSignature.h>
#include <Engine/Core/Scene.h>
#include <Engine/Core/SwapChain.h>
#include <Engine/Buffers/Texture.h>

#include <Engine/Renderer/SceneRenderer.h>
#include <Engine/Asset/TextureAssetHandler.h>
#include <Engine/Asset/MeshAssetHandler.h>

#include <d3dcompiler.h>

#include <wrl.h>
using namespace Microsoft::WRL;

#include <algorithm>
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

using namespace DirectX;

struct Mat
{
	XMMATRIX ModelMatrix;
	XMMATRIX ModelViewMatrix;
	XMMATRIX InverseTransposeModelViewMatrix;
	XMMATRIX ModelViewProjectionMatrix;
};

Tutorial::Tutorial(const std::wstring& name, int width, int height, bool vSync)
	: Game(name, width, height, vSync)
	, m_ScissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
	, m_Viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)))
	, m_Width(width)
	, m_Height(height)
	, m_ContentLoaded(false)
{
	XMVECTOR cameraPos = XMVectorSet(0, 5, -20, 1);
	XMVECTOR cameraTarget = XMVectorSet(0, 5, 0, 1);
	XMVECTOR cameraUp = XMVectorSet(0, 1, 0, 0);

	m_CameraController.m_Zoom = 10.f;
	
	auto& camera = m_Scene.GetCameraRef();
	
	camera.set_Translation(cameraPos);
	camera.set_FocalPoint(cameraTarget);

	float aspectRatio = m_Width / (float)m_Height;
	camera.set_Projection(camera.get_FoV(), aspectRatio, 1.f, 2000.0f);

	WND_PROP.Height = height;
	WND_PROP.Width = width;
	WND_PROP.Viewport = m_Viewport;
	WND_PROP.ScissorRect = m_ScissorRect;

	//m_Window = Application::Get().CreateRenderWindow(L"Testing", m_Width, m_Height);
}

Tutorial::~Tutorial()
{
}


bool Tutorial::LoadContent()
{
	// Create the DX12 device.
	m_Device = Device::Create();

	// Create a swap chain.
	m_SwapChain = m_Device->CreateSwapChain(m_pWindow->GetWindowHandle(), DXGI_FORMAT_R8G8B8A8_UNORM);
	m_SwapChain->SetVSync(true);

	m_GUI = m_Device->CreateGUI(m_pWindow->GetWindowHandle(), m_SwapChain->GetRenderTarget());

	Application::Get().WndProcHandler += WndProcEvent::slot(&GUI::WndProcHandler, m_GUI);
	// todo: setup wnd proc handler for imgui

	auto& commandQueue = m_Device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto  commandList = commandQueue.GetCommandList();

	m_Texture = TextureAssetHandler::Get().GetOrLoad("Korbo"_hs, "korbo.jpg", TL_FORCE_SRGB_SPACE);

	//m_Models.push_back(Model("Sponza"_hs, "Assets/Sponza/Sponza.gltf"));

	auto model = m_Scene.AddModel(MakeRef<Model>("Sponza"_hs, "Assets/Sponza/Sponza.gltf"));
	spdlog::info("Loaded sponza");
	model->SetScale(0.1f);
	//m_Sponza = MeshAssetHandler::Get().GetOrLoad("Sponza"_hs, "Assets/Chess/ABeautifulGame.gltf");

	//m_Cube = commandList->CreateSphere();

	commandQueue.ExecuteCommandList(commandList);

	// Create a color buffer with sRGB for gamma correction.
	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

	// Check the best multisample quality level that can be used for the given back buffer format.
	DXGI_SAMPLE_DESC sampleDesc = m_Device->GetMultisampleQualityLevels(backBufferFormat);

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = backBufferFormat;

	// Create an off-screen render target with a single color buffer and a depth buffer.
	auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(backBufferFormat, m_Width, m_Height, 1, 1, sampleDesc.Count,
		sampleDesc.Quality, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	D3D12_CLEAR_VALUE colorClearValue;
	colorClearValue.Format = colorDesc.Format;
	colorClearValue.Color[0] = 0.4f;
	colorClearValue.Color[1] = 0.6f;
	colorClearValue.Color[2] = 0.9f;
	colorClearValue.Color[3] = 1.0f;

	auto colorTexture = m_Device->CreateTexture(colorDesc, &colorClearValue);
	colorTexture->SetName(L"Color Render Target");

	// Create a depth buffer.
	auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthBufferFormat, m_Width, m_Height, 1, 1, sampleDesc.Count,
		sampleDesc.Quality, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	D3D12_CLEAR_VALUE depthClearValue;
	depthClearValue.Format = depthDesc.Format;
	depthClearValue.DepthStencil = { 1.0f, 0 };

	auto depthTexture = m_Device->CreateTexture(depthDesc, &depthClearValue);
	depthTexture->SetName(L"Depth Render Target");

	// Attach the textures to the render target.
	m_RenderTarget.AttachTexture(AttachmentPoint::Color0, colorTexture);
	m_RenderTarget.AttachTexture(AttachmentPoint::DepthStencil, depthTexture);

	commandQueue.Flush();  // Wait for loading operations to complete before rendering the first frame.

	m_SceneRenderer = MakeUnique<SceneRenderer>();
	if (!m_SceneRenderer->Initialize())
	{
		spdlog::critical("SceneRenderer failed to initialize");
		assert(false);
	}

	m_ContentLoaded = true;
	return true;
}

void Tutorial::UnloadContent()
{
	//m_Cube.reset();

	/*m_DefaultTexture.reset();
	m_DirectXTexture.reset();
	m_EarthTexture.reset();
	m_MonaLisaTexture.reset();*/

	m_RenderTarget.Reset();

	m_GUI.reset();
	m_SwapChain.reset();
	m_Device.reset();
}

void Tutorial::OnUpdate(UpdateEventArgs& e)
{
	if (!m_ContentLoaded)
	{
		return;
	}
	static uint64_t frameCount = 0;
	static double totalTime = 0.0;

	totalTime += e.DeltaTime;
	frameCount++;
	m_ElapsedTime += (float)e.DeltaTime;

	if (totalTime > 1.0)
	{
		double fps = frameCount / totalTime;

		wchar_t buffer[256];
		::swprintf_s(buffer, L"Testing [FPS: %f]", fps);
		m_pWindow->SetWindowTitle(buffer);

		frameCount = 0;
		totalTime = 0.0;
	}

	m_SwapChain->WaitForSwapChain();

	// Update the camera.
	float speedMultipler = (m_CameraController.m_Shift ? 96.f : 48.0f);

	static float m_X = 0.f;
	static float m_Y = 0.f;
	static float m_Z = 0.f;

	float scale = speedMultipler * static_cast<float>(e.DeltaTime);
	float X = scale * (m_CameraController.m_Right - m_CameraController.m_Left);
	float Y = scale * (m_CameraController.m_Up - m_CameraController.m_Down);
	float Z = scale * (m_CameraController.m_Forward - m_CameraController.m_Backward);

	Math::Smooth(m_X, X, static_cast<float>(e.DeltaTime));
	Math::Smooth(m_Y, Y, static_cast<float>(e.DeltaTime));
	Math::Smooth(m_Z, Z, static_cast<float>(e.DeltaTime));

	m_X = X;
	m_Y = Y;
	m_Z = Z;


	Camera& activeCamera = m_Scene.GetCameraRef();

	XMVECTORF32 focalPoint = { X, Y, Z };
	activeCamera.MoveFocalPoint(focalPoint, Space::Local);

	XMVECTORF32  cameraPan = { 0.0f, 0.f, -m_CameraController.m_Zoom };
	activeCamera.set_Translation(cameraPan);

	XMVECTOR cameraRotation =
		XMQuaternionRotationRollPitchYaw(XMConvertToRadians(m_CameraController.m_Pitch), XMConvertToRadians(m_CameraController.m_Yaw), 0.0f);
	activeCamera.set_Rotation(cameraRotation);

	XMMATRIX viewMatrix = activeCamera.get_ViewMatrix();

	//for (auto& model : m_Scene.GetModels())
	//{
	//	model->SetRotation(Quaternion::CreateFromYawPitchRoll((float)e.TotalTime, 0, (float)e.TotalTime));
	//}

	Game::OnUpdate(e);

	OnRender();
}

void Tutorial::OnGUI(const std::shared_ptr<CommandList>& commandList, const RenderTarget& renderTarget)
{
	m_GUI->NewFrame();

	static bool showDemoWindow = true;
	if (showDemoWindow)
	{
		ImGui::ShowDemoWindow(&showDemoWindow);
	}

	m_GUI->Render(commandList, renderTarget);
}

void XM_CALLCONV ComputeMatrices(FXMMATRIX model, CXMMATRIX view, CXMMATRIX projection, Mat& mat)
{
	mat.ModelMatrix = model;
	mat.ModelViewMatrix = model * view;
	mat.ModelViewProjectionMatrix = mat.ModelViewMatrix * projection;
	mat.InverseTransposeModelViewMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, mat.ModelViewMatrix));
}

void Tutorial::OnRender()
{
	Game::OnRender();

	auto& commandQueue = m_Device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto  commandList = commandQueue.GetCommandList();

	m_RenderTarget.Clear(commandList, { sinf(m_ElapsedTime * 1.f) * 0.5f + 0.5f, 0.8f, 0.3f, 1.0f }, true);

	//commandList->SetPipelineState(m_PipelineState);
	//commandList->SetGraphicsRootSignature(m_RootSignature);

	//commandList->SetViewport(m_Viewport);
	//commandList->SetScissorRect(m_ScissorRect);
	//commandList->SetRenderTarget(m_RenderTarget);
	
	// move this or remove it
	//commandList->SetShaderResourceView(RootParameters::Textures, 0, m_Texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	commandQueue.WaitForFenceValue(commandQueue.ExecuteCommandList(commandList));

	m_SceneRenderer->RenderScene(m_RenderTarget, m_Scene);

	commandList = commandQueue.GetCommandList();

	auto& swapChainRT = m_SwapChain->GetRenderTarget();
	auto  swapChainBackBuffer = swapChainRT.GetTexture(AttachmentPoint::Color0);
	auto  msaaRenderTarget = m_RenderTarget.GetTexture(AttachmentPoint::Color0);

	commandList->ResolveSubresource(swapChainBackBuffer, msaaRenderTarget);

	// Render the GUI directly to the swap chain's render target.
	OnGUI(commandList, swapChainRT);

	commandQueue.ExecuteCommandList(commandList);

	m_SwapChain->Present();
}

static bool g_AllowFullscreenToggle = true;
void Tutorial::OnKeyPressed(KeyEventArgs& e)
{
	Game::OnKeyPressed(e);

	if (!ImGui::GetIO().WantCaptureKeyboard)
	{
		switch (e.Key)
		{
		case KeyCode::Escape:
			Application::Get().Quit();
			break;
		case KeyCode::Enter:
			if (e.Alt)
			{
		case KeyCode::F11:
			if (g_AllowFullscreenToggle)
			{
				m_pWindow->ToggleFullscreen();
				g_AllowFullscreenToggle = false;
			}
			break;
			}
		case KeyCode::V:
			m_SwapChain->ToggleVSync();
			break;
		case KeyCode::R:
			m_CameraController.m_Zoom = 10.f;
			m_CameraController.m_Pitch = 0.0f;
			m_CameraController.m_Yaw = 0.0f;
			break;
		case KeyCode::Up:
		case KeyCode::W:
			m_CameraController.m_Forward = 1.0f;
			break;
		case KeyCode::Left:
		case KeyCode::A:
			m_CameraController.m_Left = 1.0f;
			break;
		case KeyCode::Down:
		case KeyCode::S:
			m_CameraController.m_Backward = 1.0f;
			break;
		case KeyCode::Right:
		case KeyCode::D:
			m_CameraController.m_Right = 1.0f;
			break;
		case KeyCode::Q:
			m_CameraController.m_Down = 1.0f;
			break;
		case KeyCode::E:
			m_CameraController.m_Up = 1.0f;
			break;
		case KeyCode::ShiftKey:
			m_CameraController.m_Shift = true;
			break;
		}
	}
}

void Tutorial::OnKeyReleased(KeyEventArgs& e)
{
	switch (e.Key)
	{
	case KeyCode::Enter:
		if (e.Alt)
		{
	case KeyCode::F11:
		g_AllowFullscreenToggle = true;
		}
		break;
	case KeyCode::Up:
	case KeyCode::W:
		m_CameraController.m_Forward = 0.0f;
		break;
	case KeyCode::Left:
	case KeyCode::A:
		m_CameraController.m_Left = 0.0f;
		break;
	case KeyCode::Down:
	case KeyCode::S:
		m_CameraController.m_Backward = 0.0f;
		break;
	case KeyCode::Right:
	case KeyCode::D:
		m_CameraController.m_Right = 0.0f;
		break;
	case KeyCode::Q:
		m_CameraController.m_Down = 0.0f;
		break;
	case KeyCode::E:
		m_CameraController.m_Up = 0.0f;
		break;
	case KeyCode::ShiftKey:
		m_CameraController.m_Shift = false;
		break;
	}
}

void Tutorial::OnMouseWheel(MouseWheelEventArgs& e)
{
	if (!ImGui::GetIO().WantCaptureMouse)
	{
		m_CameraController.m_Zoom -= e.WheelDelta;
		m_CameraController.m_Zoom = std::clamp(m_CameraController.m_Zoom, 1.f, 100.f);
	}
}

void Tutorial::OnMouseMoved(MouseMotionEventArgs& e)
{
	static int teleportedMouse = 0;
	const float mouseSpeed = -0.1f;

	if (!ImGui::GetIO().WantCaptureMouse)
	{
		if (e.LeftButton)
		{
			RECT clientRect;
			GetWindowRect(m_pWindow->GetWindowHandle(), &clientRect);

			POINT cursorPos;
			GetCursorPos(&cursorPos);

			if (e.X < 0)
			{
				teleportedMouse = 2;
				SetCursorPos(clientRect.left + m_Width - 3, cursorPos.y);
			}
			if (e.X > m_Width - 3)
			{
				teleportedMouse = 2;
				SetCursorPos(clientRect.left + 10, cursorPos.y);
			}
			if (e.Y < 0)
			{
				teleportedMouse = 2;
				SetCursorPos(cursorPos.x, clientRect.top + m_Height + 16);
			}
			if (e.Y > m_Height - 3)
			{
				teleportedMouse = 2;
				SetCursorPos(cursorPos.x, clientRect.top + 32);
			}
			if (teleportedMouse > 0)
			{
				teleportedMouse--;
				return;
			}

			m_CameraController.m_Pitch -= e.RelY * mouseSpeed;

			m_CameraController.m_Pitch = std::clamp(m_CameraController.m_Pitch, -90.0f, 90.0f);

			m_CameraController.m_Yaw -= e.RelX * mouseSpeed;
		}
	}
}

void Tutorial::OnResize(ResizeEventArgs& e)
{
	if (e.Width != GetClientWidth() || e.Height != GetClientHeight())
	{
		Game::OnResize(e);

		m_Width = std::max(1, e.Width);
		m_Height = std::max(1, e.Height);

		m_SwapChain->Resize(m_Width, m_Height);

		float aspectRatio = m_Width / (float)m_Height;
		m_Scene.GetCameraRef().set_Projection(m_Scene.GetCameraRef().get_FoV(), aspectRatio, 0.1f, 5000.0f);

		m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height));

		m_RenderTarget.Resize(m_Width, m_Height);
	}
}

void Tutorial::ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor)
{
	commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void Tutorial::ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth)
{
	commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

