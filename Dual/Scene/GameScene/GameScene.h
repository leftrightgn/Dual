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

class GameScene : public HEIN::IScene
{
public:

	// 更新
	void Update(GameContext& gameContext) override;

	// 描画
	void Render(GameContext& gameContext) override;

	// シーン切り替え時に呼び出される関数
	void OnEnter(GameContext& gameContext) override;

private:

	std::unique_ptr<HEIN::Skybox> m_skybox;

	DirectX::SimpleMath::Matrix m_proj;
	DirectX::SimpleMath::Matrix m_world;

	std::unique_ptr<HEIN::PhysicsSystem> m_physicsSystem;
	std::unique_ptr<HEIN::DamageSystem> m_damageSystem;

	DirectX::SimpleMath::Vector3 m_targetPos;
	DirectX::SimpleMath::Vector3 m_springEyePos;

	// --- Handle-Based Entity Memory System ---
	HEIN::ActorID m_playerID = HEIN::INVALID_ACTOR_ID;
	HEIN::ActorID m_playerSwordID = HEIN::INVALID_ACTOR_ID;
	HEIN::ActorID m_stageID = HEIN::INVALID_ACTOR_ID;
	HEIN::ActorID m_enemyID = HEIN::INVALID_ACTOR_ID;
	HEIN::ActorID m_enemySwordID = HEIN::INVALID_ACTOR_ID;
	HEIN::ActorID m_cameraID = HEIN::INVALID_ACTOR_ID;

	HEIN::ActorManager m_actorManager;

	std::unique_ptr<HEIN::DebugDisplayController> m_debugDisplay;
};