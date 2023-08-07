#include "enginepch.h"

#include <Application.h>
#include <Game.h>
#include <Window.h>

Game::Game( const std::wstring& name, int width, int height, bool vSync )
    : m_Name( name )
    , m_Width( width )
    , m_Height( height )
    , m_vSync( vSync )
{
}

Game::~Game()
{
    assert(!m_pWindow && "Use Game::Destroy() before destruction.");
}

bool Game::Initialize()
{
    // Check for DirectX Math library support.
    if (!DirectX::XMVerifyCPUSupport())
    {
        MessageBoxA(NULL, "Failed to verify DirectX Math library support.", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    m_pWindow = Application::Get().CreateRenderWindow(m_Name, m_Width, m_Height, m_vSync);
   // m_pWindow->RegisterCallbacks(shared_from_this());
    m_pWindow->Update += UpdateEvent::slot(&Game::OnUpdate, this);
    m_pWindow->KeyPressed += KeyboardEvent::slot(&Game::OnKeyPressed, this);
    m_pWindow->KeyReleased += KeyboardEvent::slot(&Game::OnKeyReleased, this);
    m_pWindow->MouseMoved += MouseMotionEvent::slot(&Game::OnMouseMoved, this);
    m_pWindow->MouseWheel += MouseWheelEvent::slot(&Game::OnMouseWheel, this);
    m_pWindow->Resize += ResizeEvent::slot(&Game::OnResize, this);
    m_pWindow->Show();

    return true;
}

void Game::Destroy()
{
    m_pWindow.reset();
}

void Game::OnUpdate(UpdateEventArgs&)
{

}

void Game::OnRender()
{

}

void Game::OnKeyPressed(KeyEventArgs&)
{
    // By default, do nothing.
}

void Game::OnKeyReleased(KeyEventArgs&)
{
    // By default, do nothing.
}

void Game::OnMouseMoved(class MouseMotionEventArgs&)
{
    // By default, do nothing.
}

void Game::OnMouseButtonPressed(MouseButtonEventArgs&)
{
    // By default, do nothing.
}

void Game::OnMouseButtonReleased(MouseButtonEventArgs&)
{
    // By default, do nothing.
}

void Game::OnMouseWheel(MouseWheelEventArgs&)
{
    // By default, do nothing.
}

void Game::OnResize(ResizeEventArgs& e)
{
    m_Width = e.Width;
    m_Height = e.Height;
}

void Game::OnWindowDestroy()
{
    // If the Window which we are registered to is 
    // destroyed, then any resources which are associated 
    // to the window must be released.
    UnloadContent();
}

