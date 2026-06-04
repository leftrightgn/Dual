#pragma once
#include "ColliderComponent.h"
#include "DirectXCollision.h"
#include <Framework/GameContext.h>

namespace HEIN
{
	class StaticModelComponent;

	class OBBColliderComponent : public ColliderComponent
	{
	private:

		DirectX::SimpleMath::Vector3 m_extents;

	public:

		OBBColliderComponent(Actor* owner);
		virtual ~OBBColliderComponent() = default;

		void Initialize(const DirectX::SimpleMath::Vector3 extents);
		void InitializeFromModel(StaticModelComponent* staticModel);

		void Update(float deltaTime) override;
		void Draw(
			GameContext& gameContext,
			const DirectX::SimpleMath::Matrix& world,
			const DirectX::SimpleMath::Matrix& view,
			const DirectX::SimpleMath::Matrix& proj
		) override;
		DirectX::SimpleMath::Vector3 GetExtents() { return m_extents; }

	};
}