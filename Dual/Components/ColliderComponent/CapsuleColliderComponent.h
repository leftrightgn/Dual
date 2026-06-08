#pragma once
#include "ColliderComponent.h"

namespace HEIN
{
	class StaticModelComponent;

	class CapsuleColliderComponent : public ColliderComponent
	{
	private:

		float m_radius;
		float m_height;

	public:

		CapsuleColliderComponent(Actor* owner);
	

		void Initialize(float radius, float height);

		void Update(float deltaTime) override;
		void Draw(
			GameContext& gameContext,
			const DirectX::SimpleMath::Matrix& world,
			const DirectX::SimpleMath::Matrix& view,
			const DirectX::SimpleMath::Matrix& proj
		) override;

		float GetRadius() const { return m_radius; }
		float GetHeight() const { return m_height; }

		void SetRadius(float radius) { m_radius = radius; }
		void SetHeight(float height) { m_height = height; }
	};


}

