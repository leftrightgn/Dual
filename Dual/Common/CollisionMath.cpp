#include "pch.h"
#include "CollisionMath.h"
#include <Components/ColliderComponent/AABBColliderComponent.h>
#include <Components/ColliderComponent/ColliderComponent.h>
#include <Components/ColliderComponent/CapsuleColliderComponent.h>
#include <Components/ColliderComponent/OBBColliderComponent.h>

HEIN::CollisionManifold HEIN::CollisionMath::CheckCapsuleVsOBB(HEIN::CapsuleColliderComponent* capsule, HEIN::OBBColliderComponent* obb)
{
    HEIN::CollisionManifold manifold;
    // TO DO::logic
    return manifold;
}

HEIN::CollisionManifold HEIN::CollisionMath::CheckOBBvsOBB(HEIN::OBBColliderComponent* obbA, HEIN::OBBColliderComponent* obbB)
{
    HEIN::CollisionManifold maniflod;
    // TO DO::logic 
    return maniflod;
}

HEIN::CollisionManifold HEIN::CollisionMath::CheckCapsuleVsAABB(HEIN::CapsuleColliderComponent* capsule, HEIN::AABBColliderComponent* aabb)
{
    HEIN::CollisionManifold manifold;
    manifold.isColliding = false;   

    if (capsule == nullptr || aabb == nullptr) return manifold;

    DirectX::SimpleMath::Vector3 capPos = capsule->GetCalculateWorldMatrix().Translation();
    DirectX::SimpleMath::Matrix aabbWorld = aabb->GetCalculateWorldMatrix();
    DirectX::SimpleMath::Vector3 aabbPos = aabbWorld.Translation();
     DirectX::SimpleMath::Vector3 scale(
        DirectX::SimpleMath::Vector3(aabbWorld._11, aabbWorld._12, aabbWorld._13).Length(),
        DirectX::SimpleMath::Vector3(aabbWorld._21, aabbWorld._22, aabbWorld._23).Length(),
        DirectX::SimpleMath::Vector3(aabbWorld._31, aabbWorld._32, aabbWorld._33).Length()
    );

    // 2. MULTIPLY EXTENTS BY SCALE
    DirectX::SimpleMath::Vector3 extents = aabb->GetExtents() * scale;
    float halfHeight = capsule->GetHeight() * 0.5f;

    DirectX::SimpleMath::Vector3 SegTop(capPos.x, capPos.y + halfHeight, capPos.z);
    DirectX::SimpleMath::Vector3 SegBottom(capPos.x, capPos.y - halfHeight, capPos.z);

    auto clampToAABB = [&](const DirectX::SimpleMath::Vector3& p)->DirectX::SimpleMath::Vector3
        {
            return DirectX::SimpleMath::Vector3(
                std::fmax(aabbPos.x - extents.x, std::min(p.x, aabbPos.x + extents.x)),
                std::fmax(aabbPos.y - extents.y, std::min(p.y, aabbPos.y + extents.y)),
                std::fmax(aabbPos.z - extents.z, std::min(p.z, aabbPos.z + extents.z))

            );
        };

    DirectX::SimpleMath::Vector3 closetToTop = clampToAABB(SegTop);
    DirectX::SimpleMath::Vector3 closetToBottom = clampToAABB(SegBottom);

    DirectX::SimpleMath::Vector3 diffTop = SegTop - closetToTop;
    DirectX::SimpleMath::Vector3 diffBottom = SegBottom - closetToBottom;

    DirectX::SimpleMath::Vector3 distanceVector;
    float distance;

    if (diffBottom.LengthSquared() <= diffTop.LengthSquared())
    {
        distanceVector = diffBottom;
        distance = diffBottom.Length();
    }
    else
    {
        distanceVector = diffTop;
        distance = diffTop.Length();
    }

    if (distance < capsule->GetRadius())
    {
        manifold.isColliding = true;
        manifold.penetrationDepth = capsule->GetRadius() - distance;

        if (distance > 0.001f)
        {
            distanceVector.Normalize();

            manifold.normal = distanceVector;
        }
        else
        {
            manifold.normal = DirectX::SimpleMath::Vector3(0.0f, 1.0f, 0.0f);
        }
    }

    return manifold;
}