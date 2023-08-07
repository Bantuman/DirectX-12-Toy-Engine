#pragma once

#include <Engine/Core/Game.h>
#include <Engine/Core/Window.h>
#include <Engine/Core/Camera.h>
#include <Engine/Core/Scene.h>
#include <Engine/Core/RenderTarget.h>

class SwapChain;
class RootSignature;
class Device;
class GUI;
class PipelineStateObject;
class SceneRenderer;
class CommandList;
class Mesh;

class Tutorial : public Game
{
public:
    Tutorial(const std::wstring& name, int width, int height, bool vSync);
    ~Tutorial();

    virtual bool LoadContent() override;
    virtual void UnloadContent() override;

protected:
    void OnUpdate(UpdateEventArgs& e) override;
    void OnRender() override;
    void OnKeyPressed(KeyEventArgs& e) override;
    void OnKeyReleased(KeyEventArgs& e) override;
    void OnMouseWheel(MouseWheelEventArgs& e) override;
    void OnMouseMoved(MouseMotionEventArgs& e) override;
    void OnResize(ResizeEventArgs& e) override;
    void OnGUI(const std::shared_ptr<CommandList>& commandList, const RenderTarget& renderTarget);

private:

    void ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
        D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor);

    void ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
        D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth = 1.0f);

    UniquePtr<SceneRenderer> m_SceneRenderer;

    // Depth buffer.
    Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthBuffer;

    RefPtr<Device> m_Device;
    RefPtr<SwapChain> m_SwapChain;
    RefPtr<GUI> m_GUI;

    RefPtr<Texture> m_Texture;

    Scene m_Scene;

    RenderTarget m_RenderTarget;

    D3D12_VIEWPORT m_Viewport;
    D3D12_RECT m_ScissorRect;

    struct CameraController
    {
        bool m_Shift;
        float m_Forward;
        float m_Backward;
        float m_Left;
        float m_Right;
        float m_Up;
        float m_Down;

        float m_Zoom;
        float m_Pitch;
        float m_Yaw;
    } m_CameraController{};

    int  m_Width;
    int  m_Height;
    float m_ElapsedTime = 0;

    DirectX::XMMATRIX m_ModelMatrix;
    DirectX::XMMATRIX m_ViewMatrix;
    DirectX::XMMATRIX m_ProjectionMatrix;

    bool m_ContentLoaded;
};