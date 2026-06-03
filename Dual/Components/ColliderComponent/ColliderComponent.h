#pragma once
#include "Components/IComponent.h"
#include <string>

namespace HEIN
{
	enum class ColliderShape
	{
		Sphere,
		AABB,
		OBB
	};

	class SkinnedModelComponent;

	class ColliderComponent : public IComponent
	{
	protected:
		
		DirectX::SimpleMath::Vector3 m_offset;
		ColliderShape m_shape;
		bool m_isTrigger;

		SkinnedModelComponent* m_skinnedModel = nullptr;
		std::wstring m_targetBoneName = L"";
		int m_targetBoneNum = -1;

	public:

		ColliderComponent(Actor* owner, ColliderShape shape);
		virtual ~ColliderComponent() = default;

		virtual void Update(float deltaTime) override = 0;
		virtual void DrawDebug(GameContext& gameContext) = 0;

		void AttachToBone(SkinnedModelComponent* model, const std::wstring& boneName);
		void AttachToBone(SkinnedModelComponent* model, const int boneNum);

		ColliderShape GetShape() { return m_shape; }
		void SetTrigger(bool active) { m_isTrigger = active; }
		bool IsTrigger() const { return m_isTrigger; }

	};
}
