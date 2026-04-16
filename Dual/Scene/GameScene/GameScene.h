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
#include "GameContext.h"
#include "../SceneId.h"
#include "Effect/Water.h"
#include "Effect/SkyboxEffect.h"
#include "ImaseLib/DebugCamera.h"
#include "DirectXTK_Utilities/Animation.h"

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

	std::unique_ptr<DirectX::GeometricPrimitive> m_sky;

	std::unique_ptr<DX::SkyboxEffect> m_effect;

	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_skyInputLayout;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_cubemap;

	std::unique_ptr<Water> m_water;

	std::unique_ptr<Imase::DebugCamera> m_debugCamera;

	DirectX::SimpleMath::Matrix m_proj;

	DirectX::SimpleMath::Matrix m_world;

	std::unique_ptr<DirectX::EffectFactory> m_fxFactory;

	std::unique_ptr<DirectX::Model> m_model;

	DirectX::ModelBone::TransformArray m_drawBones;

	DX::AnimationSDKMESH m_animation;
};


