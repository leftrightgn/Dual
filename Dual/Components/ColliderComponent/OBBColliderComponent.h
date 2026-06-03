#pragma once
#include "ColliderComponent.h"
#include "DirectXCollision.h"

namespace HEIN
{
	class OBBColliderComponent : public ColliderComponent
	{
	private:

		DirectX::SimpleMath::Vector3 m_extents;

	public:

		OBBColliderComponent(Actor* owner);
		virtual ~OBBColliderComponent() = default;

		void Initialize(const DirectX::SimpleMath::Vector3 extents);

		void Update(float deltaTime) override;
		void DrawDebug(GameContext& gameContext) override;

		DirectX::SimpleMath::Vector3 GetExtents() { return m_extents; }

	};
}