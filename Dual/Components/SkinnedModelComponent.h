#pragma once
#include "IComponent.h"
#include "DirectXTK_Utilities/Animation.h"

namespace HEIN
{
	class SkinnedModelComponent : public IComponent
	{
	private:

		// Model And Animation Data 
		std::unique_ptr<DirectX::Model> m_model;
		std::unique_ptr<DirectX::EffectFactory> m_fxFactory;
		DirectX::ModelBone::TransformArray m_drawBones;
		DirectX::ModelBone::TransformArray m_skinBones;
		//DX::AnimationSDKMESH m_animation;
		std::unordered_map < std::string, std::unique_ptr<DX::AnimationSDKMESH>> m_animations;

		DX::AnimationSDKMESH* m_currentAnimation = nullptr;

		bool m_isVisible = true;

	public:

		SkinnedModelComponent(Actor* owner);

		void Initialize(
			GameContext& gameContext, 
			const wchar_t* modelPath, 
			const wchar_t* textureDir
		);

		void Update(float deltaTime) override;

		void Draw(
			GameContext& gameContext,
			const DirectX::SimpleMath::Matrix& world, 
			const DirectX::SimpleMath::Matrix& view,
			const DirectX::SimpleMath::Matrix& proj
		);

		DirectX::SimpleMath::Vector3 GetBoneWorldPosition(
			const wchar_t* boneName,
			const DirectX::SimpleMath::Matrix& actorWorldMatrix
		);
		DirectX::SimpleMath::Vector3 GetBoneWorldPosition(
			const int boneNum,
			const DirectX::SimpleMath::Matrix& actorWorldMatrix
		);

		DirectX::SimpleMath::Matrix GetBoneWorldMatrix(
			const wchar_t* boneName,
			const DirectX::SimpleMath::Matrix& actorWorldMatrix
		);

		void LoadAnimation(const std::string& name, const wchar_t* animPath);
		void ChangeAnimation(const std::string& name);

		void SetVisible(bool visible) { m_isVisible = visible; }
		bool IsVisible() const { return m_isVisible; }


	};
}

