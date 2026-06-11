#pragma once


namespace HEIN
{
	class CapsuleColliderComponent;
	class OBBColliderComponent;
	class AABBColliderComponent;

	struct CollisionManifold
	{
		bool isColliding = false;
		DirectX::SimpleMath::Vector3 normal = DirectX::SimpleMath::Vector3::Zero; // The direction to push out
		float penetrationDepth = 0.0f;                                            // How far to push out
	};

	class CollisionMath
	{
	public:

		static HEIN::CollisionManifold CheckCapsuleVsOBB(HEIN::CapsuleColliderComponent* capsule, HEIN::OBBColliderComponent* obb);

		static HEIN::CollisionManifold CheckOBBvsOBB(HEIN::OBBColliderComponent* obbA, HEIN::OBBColliderComponent* obbB);

		static HEIN::CollisionManifold CheckCapsuleVsAABB(HEIN::CapsuleColliderComponent* capsule, HEIN::AABBColliderComponent* aabb);
	};
}
