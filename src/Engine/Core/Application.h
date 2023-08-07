/**
* The application class is used to create windows for our application.
*/
#pragma once

#include <dxgi1_6.h>
#include <Engine/Memory/Descriptors/DescriptorAllocation.h>
#include <string>
#include <Engine/Core/Device.h>
#include <Engine/Core/Events.h>

#include <spdlog/spdlog.h>

using Logger = RefPtr<spdlog::logger>;

class CommandQueue;
class DescriptorAllocator;
class Game;
class Window;

class TextureAssetHandler;
class MeshAssetHandler;

using WndProcEvent = _Delegate<LRESULT(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)>;

struct ActiveWindowProperties
{
    u32 Width;
    u32 Height;
    D3D12_VIEWPORT Viewport;
    D3D12_RECT ScissorRect;
};

#define WND_PROP Application::WindowProperties

class Application
{
public:
    static ActiveWindowProperties WindowProperties;

    /**
    * Create the application singleton with the application instance handle.
    */
    static void Create(HINSTANCE hInst);

    /**
    * Destroy the application instance and all windows created by this application instance.
    */
    static void Destroy();
    /**
    * Get the application singleton.
    */
    static Application& Get();

    /**
     * Check to see if VSync-off is supported.
     */
    bool IsTearingSupported() const;

    /**
    * Create a new DirectX11 render window instance.
    * @param windowName The name of the window. This name will appear in the title bar of the window. This name should be unique.
    * @param clientWidth The width (in pixels) of the window's client area.
    * @param clientHeight The height (in pixels) of the window's client area.
    * @param vSync Should the rendering be synchronized with the vertical refresh rate of the screen.
    * @param windowed If true, the window will be created in windowed mode. If false, the window will be created full-screen.
    * @returns The created window instance. If an error occurred while creating the window an invalid
    * window instance is returned. If a window with the given name already exists, that window will be
    * returned.
    */
    RefPtr<Window> CreateRenderWindow(const std::wstring& windowName, int clientWidth, int clientHeight, bool vSync = true );

    /**
    * Find a window by the window name.
    */
    RefPtr<Window> GetWindowByName(const std::wstring& windowName);

    /**
    * Run the application loop and message pump.
    * @return The error code if an error occurred.
    */
    int Run(RefPtr<Game> pGame);

    /**
    * Request to quit the application and close all windows.
    * @param exitCode The error code to return to the invoking process.
    */
    void Quit(int exitCode = 0);

    /**
     * Get the Direct3D 12 device
     */
    Microsoft::WRL::ComPtr<ID3D12Device2> GetD3D12Device() const;

    /**
    * Create a named logger or get a previously created logger with the same
    * name.
    */
    Logger CreateLogger(const std::string& name);

    /**
    * Get the wrapped device
    */
    RefPtr<Device> GetDevice() const;

    /**
     * Invoked when a message is sent to a window.
     */
    WndProcEvent WndProcHandler;

    static u64 GetFrameCount()
    {
        return ms_FrameCount;
    }

    static inline u64 ms_FrameCount;
protected:
    friend LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    // Create an application instance.
    Application(HINSTANCE hInst);
    // Destroy the application instance and all windows associated with this application.
    virtual ~Application();

    // Windows message handler.
    virtual LRESULT OnWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


    bool CheckTearingSupport();

private:
    Application(const Application& copy) = delete;
    Application& operator=(const Application& other) = delete;

    // The application instance handle that this application was created with.
    HINSTANCE m_hInstance;

    RefPtr<TextureAssetHandler> m_TextureAssetHandler;
    RefPtr<MeshAssetHandler> m_MeshAssetHandler;

    RefPtr<Device> m_Device;
    Logger m_Logger;

    bool m_TearingSupported;

    // Set to true while the application is running.
    std::atomic_bool m_bIsRunning;
    // Should the application quit?
    std::atomic_bool m_RequestQuit;
};