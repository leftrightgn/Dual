//--------------------------------------------------------------------------------------
// File: GameScene.h
//
// 新規シーン作成時の元にするファイル
//
// Date: 2026.4.13
// Author: Hideyasu Imase
//--------------------------------------------------------------------------------------
#pragma once
#include "pch.h"
#include "ImaseLib/SceneManager.h"
#include "Framework/GameContext.h"
#include "../SceneId.h"
#include "Effect/Water.h"
#include "Effect/Skybox.h"
#include "ImaseLib/DebugCamera.h"
#include "Camera/CameraController.h"
#include <Entities/Actor.h>
#include "Common/DebugDisplayController.h"



class GameScene : public Imase::SceneBase<SceneId, GameContext>
{
public:

	// 更新
	void Update(Imase::ISceneController<SceneId>& sceneController, GameContext& gameContext) override;

	// 描画
	void Render(GameContext& gameContext) override;

	// シーン切り替え時に呼び出される関数
	void OnEnter(GameContext& gameContext) override;

private:

	std::unique_ptr<HEIN::Skybox> m_skybox;

	std::unique_ptr<Water> m_water;

	std::unique_ptr<Imase::DebugCamera> m_debugCamera;

	DirectX::SimpleMath::Matrix m_proj;

	DirectX::SimpleMath::Matrix m_world;


	std::unique_ptr<HEIN::CameraController> m_cameraController;


	DirectX::SimpleMath::Vector3 m_targetPos;
	DirectX::SimpleMath::Vector3 m_springEyePos;

	HEIN::Actor* m_player = nullptr;
	HEIN::Actor* m_swordActor = nullptr;
	HEIN::Actor* m_stageActor = nullptr;
	std::vector<std::unique_ptr<HEIN::Actor>> m_actors;

	std::unique_ptr<DirectX::GeometricPrimitive> m_debugSphere;

	std::unique_ptr<HEIN::DebugDisplayController> m_debugDisplay;
};


