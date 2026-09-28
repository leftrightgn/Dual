//--------------------------------------------------------------------------------------
// File: GameScene.h
// Author: HEIN SOE KHANT
//--------------------------------------------------------------------------------------
#pragma once
#include "pch.h"
#include "../../../External/Engine/Entities/Actor.h"
#include "../../../External/Engine/Entities/ActorManager.h"
#include "../../../External/Engine/Scene/IScene.h"
#include "../../../External/Engine/Effect/Skybox.h"
#include "../../../External/Engine/Common/PhysicsSystem.h"
#include "../../../External/Engine/Common/DamageSystem.h"
#include "../../../External/Engine/DebugingTools/DebugDisplayController.h"
#include "../../../External/Engine/Common/ShadowSystem.h"

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
	void Update(GameContext& gameContext) override;
	void Render(GameContext& gameContext) override;

private:

	// --- ECS Update Pipeline Phases ---
	void ProcessInputPhase(GameContext& gameContext);
	void ProcessSimulationPhase(GameContext& gameContext, float deltaTime);
	void ProcessLifecyclePhase();

	// --- Render Pipeline Phases ---
	void RenderShadowPhase(GameContext& gameContext, ID3D11DeviceContext* context);
	void RenderMainPassPhase(GameContext& gameContext, ID3D11DeviceContext* context, const DirectX::SimpleMath::Matrix& view);
	void RenderUIPhase();

	// --- Editor & Scene Management ---
	void HandleEditorActions(GameContext& gameContext);
	void LoadAutoSave(GameContext& gameContext);
	void UpdateDebugTargets();

	// --- Entity & Component Accessors ---
	HEIN::Actor* GetPlayerActor();
	HEIN::Actor* GetEnemyActor();
	HEIN::CameraController* GetActiveCameraController(GameContext& gameContext);

	// --- Setup Helpers ---
	void SetupCameraModes(GameContext& gameContext, HEIN::CameraController* cameraComp, HEIN::SkinnedModelComponent* modelPointer);

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

	HEIN::ActorManager m_actorManager;
	HEIN::ShadowSystem m_shadowSystem;

	std::unique_ptr<HEIN::DebugDisplayController> m_debugDisplay;
	bool m_isPlaying = true;
};