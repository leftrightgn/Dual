//--------------------------------------------------------------------------------------
// File: TitleScene.h
// Author: HEIN SOE KHANT
//--------------------------------------------------------------------------------------
#pragma once
#include "pch.h"
#include "../../../External/Engine/Entities/Actor.h"
#include "../../../External/Engine/Entities/ActorManager.h"
#include "../../../External/Engine/Scene/IScene.h"
#include "../../../External/Engine/Effect/Skybox.h"
#include "../../../External/Engine/Components/UIButtonComponent.h"
#include "../../../External/Engine/Camera/CameraController.h"
#include "../../../External/Engine/DebugingTools/DebugDisplayController.h"

/**
 * @class TitleScene
 * @brief Handles the initial start menu, UI interactions, and scene transitions.
 * 
 * Manages the "Start Game" and "Quit Game" buttons, as well as a debug/editor
 * interface for tweaking the title layout and automatically saving changes.
 */
class TitleScene : public HEIN::IScene
{
public:
    TitleScene() = default;
    ~TitleScene() override = default;

    void OnEnter(GameContext& gameContext) override;
    void OnExit(GameContext& gameContext) override;
    void Update(GameContext& gameContext) override;
    void Render(GameContext& gameContext) override;

private:
    std::unique_ptr<HEIN::Skybox> m_skybox;

    DirectX::SimpleMath::Matrix m_proj;
    DirectX::SimpleMath::Matrix m_world;

    HEIN::ActorManager m_actorManager;

    HEIN::ActorID m_cameraID = HEIN::INVALID_ACTOR_ID;
    HEIN::ActorID m_startButtonID = HEIN::INVALID_ACTOR_ID;
    HEIN::ActorID m_exitButtonID = HEIN::INVALID_ACTOR_ID;

    HEIN::UIButtonComponent* m_startButton = nullptr;
    HEIN::UIButtonComponent* m_exitButton = nullptr;
    HEIN::CameraController* m_cameraController = nullptr;

    std::unique_ptr<HEIN::DebugDisplayController> m_debugDisplay;
    bool m_isPlaying = true;

    float m_titlePulseTimer = 0.0f;
    float m_autoSaveTimer = 0.0f;
};
