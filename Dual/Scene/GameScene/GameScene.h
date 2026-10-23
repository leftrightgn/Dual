//--------------------------------------------------------------------------------------
// File: GameScene.h
// Author: HEIN SOE KHANT
//--------------------------------------------------------------------------------------
#pragma once
#include "pch.h"
#include "../../External/Engine/System/DamageSystem.h"
#include "../../External/Engine/System/PhysicsSystem.h"
#include "../../External/Engine/System/ShadowSystem.h"
#include "../../External/Engine/DebugingTools/DebugDisplayController.h"
#include "../../External/Engine/Effect/Skybox.h"
#include "../../External/Engine/Entities/Actor.h"
#include "../../External/Engine/Entities/ActorManager.h"
#include "../../External/Engine/Scene/IScene.h"

enum class HitboxDebugScope
{
	Off = 0,
	ActiveAttacksOnly = 1,
	AllColliders = 2
};

#include "../../Components/GameFlowStateMachineComponent.h"
#include "../../Components/EncounterTriggerComponent.h"
using HEIN::GameFlowState;

namespace HEIN
{
	class CameraController;
	class SkinnedModelComponent;
}

/**
 * @class GameScene
 * @brief Coordinates the gameplay world, executing ECS pipeline phases.
 * 
 * Orchestrates entity lifecycles, physics simulation, camera tracking,
 * and rendering passes through clean system phases.
 */
class GameScene : public HEIN::IScene
{
public:

	void OnEnter(GameContext& gameContext) override;
	void OnExit(GameContext& gameContext) override;
	void Update(GameContext& gameContext) override;
	void Render(GameContext& gameContext) override;

private:

	// --- ECS Update Pipeline Phases ---
	void ProcessInputPhase(GameContext& gameContext);
	void ProcessSimulationPhase(GameContext& gameContext, float deltaTime);
	void ProcessLifecyclePhase(GameContext& gameContext, float deltaTime);

	// --- Render Pipeline Phases ---
	void RenderShadowPhase(GameContext& gameContext, ID3D11DeviceContext* context);
	void RenderMainPassPhase(GameContext& gameContext, ID3D11DeviceContext* context, const DirectX::SimpleMath::Matrix& view);

	// --- Editor & Scene Management ---
	void OpenEditor();
	void UnloadEditor(GameContext& gameContext);
	void HandleEditorActions(GameContext& gameContext);
	void LoadSceneData(GameContext& gameContext);
	void UpdateDebugTargets();

	// --- Entity & Component Accessors ---
	float GetDeathAnimDuration() const { return m_deathAnimDuration; }
	void SetDeathAnimDuration(float duration) { m_deathAnimDuration = duration; }
	HEIN::Actor* GetPlayerActor();
	HEIN::Actor* GetEnemyActor();
	HEIN::CameraController* GetActiveCameraController(GameContext& gameContext);

	// --- Setup Helpers ---
	void SetupCameraModes(GameContext& gameContext, HEIN::CameraController* cameraComp, HEIN::SkinnedModelComponent* modelPointer);
	void PostSceneLoadInit(GameContext& gameContext);
	void TriggerVictory(GameContext& gameContext);
	void TriggerGameOver(GameContext& gameContext);
	void ChangeFlowState(GameFlowState newState, GameContext& gameContext);
	void ResetMatch(GameContext& gameContext);

	HEIN::GameFlowStateMachineComponent* GetGameFlowComponent();
	HEIN::EncounterTriggerComponent* GetActiveEncounterComponent();

private:

	std::unique_ptr<HEIN::Skybox> m_skybox;

	DirectX::SimpleMath::Matrix m_proj;
	DirectX::SimpleMath::Matrix m_world;

	std::unique_ptr<HEIN::PhysicsSystem> m_physicsSystem;
	std::unique_ptr<HEIN::DamageSystem> m_damageSystem;

	DirectX::SimpleMath::Vector3 m_targetPos;
	DirectX::SimpleMath::Vector3 m_springEyePos;

	// Cached Entity IDs for quick O(1) lookup
	HEIN::ActorID m_playerID = HEIN::INVALID_ACTOR_ID;
	HEIN::ActorID m_enemyID = HEIN::INVALID_ACTOR_ID;
	HEIN::ActorID m_cameraID = HEIN::INVALID_ACTOR_ID;

	bool m_isGhost = false;
	bool m_isDying = false;
	float m_deathFreezeTimer = 0.0f;
	int m_deathFreezeFrames = 15;
	bool m_isDeathAnimPlaying = false;
	float m_deathAnimTimer = 0.0f;
	float m_deathAnimDuration = 3.90f;
	HEIN::ActorID m_targetVesselID = HEIN::INVALID_ACTOR_ID;

	HEIN::ActorManager m_actorManager;
	HEIN::ShadowSystem m_shadowSystem;

	std::unique_ptr<HEIN::DebugDisplayController> m_debugDisplay;
	bool m_isPlaying = true;
	HitboxDebugScope m_hitboxDebugScope = HitboxDebugScope::Off;

	// Elden Ring Camera Features
	float m_manualUnlockCooldown = 0.0f;

	// Game Flow State Machine
	GameFlowState m_flowState = GameFlowState::Gameplay;
	float m_flowTimer = 0.0f;
	bool m_hasPlayedIntro = false;
	bool m_isVictory = false;
	float m_victoryTimer = 0.0f;
	HEIN::ActorID m_victoryBannerID = HEIN::INVALID_ACTOR_ID;
	HEIN::ActorID m_backToTitleButtonID = HEIN::INVALID_ACTOR_ID;
	HEIN::ActorID m_retryButtonID = HEIN::INVALID_ACTOR_ID;
	HEIN::ActorID m_gameOverBannerID = HEIN::INVALID_ACTOR_ID;

	HEIN::ActorID m_gameManagerID = HEIN::INVALID_ACTOR_ID;
	HEIN::ActorID m_activeEncounterActorID = HEIN::INVALID_ACTOR_ID;
};
