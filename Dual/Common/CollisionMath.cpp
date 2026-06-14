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

    DirectX::SimpleMath::Vector3 extents = aabb->GetExtents() * scale;
    float halfHeight = capsule->GetHeight() * 0.5f;

    // Capsule segment
    DirectX::SimpleMath::Vector3 SegTop(capPos.x, capPos.y + halfHeight, capPos.z);
    DirectX::SimpleMath::Vector3 SegBottom(capPos.x, capPos.y - halfHeight, capPos.z);
    DirectX::SimpleMath::Vector3 d = SegTop - SegBottom;
    float len = d.Length();
    
    // Clamp point to AABB
    auto clampToAABB = [&](const DirectX::SimpleMath::Vector3& p) -> DirectX::SimpleMath::Vector3 {
        return DirectX::SimpleMath::Vector3(
            std::fmax(aabbPos.x - extents.x, std::min(p.x, aabbPos.x + extents.x)),
            std::fmax(aabbPos.y - extents.y, std::min(p.y, aabbPos.y + extents.y)),
            std::fmax(aabbPos.z - extents.z, std::min(p.z, aabbPos.z + extents.z))
        );
    };

    // Find the closest point on the segment to the AABB
    // We can iteratively approach the closest point by projecting the clamped point back onto the segment
    DirectX::SimpleMath::Vector3 closestOnSeg = capPos; // Start at center
    DirectX::SimpleMath::Vector3 closestOnAABB = clampToAABB(closestOnSeg);
    
    // Perform a few iterations to find the closest points
    for (int i = 0; i < 3; ++i) {
        // Project closestOnAABB onto the segment
        DirectX::SimpleMath::Vector3 toAABB = closestOnAABB - SegBottom;
        float t = (len > 0.0001f) ? toAABB.Dot(d) / (len * len) : 0.5f;
        t = std::fmax(0.0f, std::fmin(1.0f, t));
        closestOnSeg = SegBottom + d * t;
        
        // Update closestOnAABB
        closestOnAABB = clampToAABB(closestOnSeg);
    }

    DirectX::SimpleMath::Vector3 diff = closestOnSeg - closestOnAABB;
    float distance = diff.Length();

    if (distance < capsule->GetRadius())
    {
        manifold.isColliding = true;
        manifold.penetrationDepth = capsule->GetRadius() - distance;

        if (distance > 0.001f)
        {
            diff.Normalize();
            manifold.normal = diff;
        }
        else
        {
            // If centers are identical, use a default normal (e.g., up)
            manifold.normal = DirectX::SimpleMath::Vector3(0.0f, 1.0f, 0.0f);
        }
    }

    return manifold;
}