//
// Game.h
//

#pragma once

#include "Common/DeviceResources.h"
#include "Common/StepTimer.h"

#include <memory>
#include <optional>

#include "ImaseLib/SceneManager.h"
#include "GameContext.h"
#include "Scene/SceneId.h"

// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game final : public DX::IDeviceNotify
{
public:

    Game() noexcept(false);
    ~Game() = default;

    Game(Game&&) = default;
    Game& operator= (Game&&) = default;

    Game(Game const&) = delete;
    Game& operator= (Game const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic game loop
    void Tick();

    // IDeviceNotify
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnDisplayChange();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize( int& width, int& height ) const noexcept;

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>    m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                           m_timer;

    // --------------------------------------------------------------------- //

private:

    // キーボードトラッカー
    DirectX::Keyboard::KeyboardStateTracker m_keyboardTracker;

    // マウスボタントラッカー
    DirectX::Mouse::ButtonStateTracker m_mouseButtonTracker;

    // コモンステート
    std::unique_ptr<DirectX::CommonStates> m_states;

    // デバッグ用の描画セット
    std::unique_ptr<Imase::DebugRenderer> m_debugRenderer;

    // ゲームコンテキスト
    std::optional<GameContext> m_gameContext;

    // シーンマネージャー
    Imase::SceneManager<SceneId, GameContext> m_sceneManager;

};
