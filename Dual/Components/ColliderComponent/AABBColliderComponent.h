#pragma once
#include "ColliderComponent.h"

namespace HEIN
{
	class StaticModelComponent;

	class AABBColliderComponent : public ColliderComponent
	{
	private:

		DirectX::SimpleMath::Vector3 m_extents;

	public:

		AABBColliderComponent(Actor* owner);
	
		void Initialize(const DirectX::SimpleMath::Vector3 extents);
		void InitializeFromModel(StaticModelComponent* staticModel);

		void Update(float deltaTime) override;
		void Draw(
			GameContext& gameContext,
			const DirectX::SimpleMath::Matrix& world,
			const DirectX::SimpleMath::Matrix& view,
			const DirectX::SimpleMath::Matrix& proj
		) override;

		DirectX::SimpleMath::Vector3 GetExtents() const { return m_extents; }
	};


}

