#pragma once
#include "ColliderComponent.h"
#include <Common/CollisionMath.h>
#include <vector> 

namespace HEIN
{
	class MeshColliderComponent : public ColliderComponent
	{
	private:

		std::vector<Triangle> m_localTriangles;
		std::vector<Triangle> m_worldTriangles;

	public:

		MeshColliderComponent(Actor* owner);

		void LoadFromObj(const wchar_t* filePath);

		void Update(float /*deltaTime*/) override {}

		void SyncColliderState() override;

		void Draw(
			GameContext& gameContext,
			const DirectX::SimpleMath::Matrix& world,
			const DirectX::SimpleMath::Matrix& view,
			const DirectX::SimpleMath::Matrix& proj
		) override {}

		const std::vector<Triangle>& GetWorldTriangles() const{ return m_worldTriangles; }
	};
}


