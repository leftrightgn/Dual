#include "pch.h"
#include "OBBColliderComponent.h"
#include "Components/StaticModelComponent.h"
#include <DirectXColors.h>

HEIN::OBBColliderComponent::OBBColliderComponent(Actor* owner)
	: ColliderComponent(owner, ColliderShape::OBB)
	, m_extents(DirectX::SimpleMath::Vector3::Zero)
{
}

void HEIN::OBBColliderComponent::Initialize(const DirectX::SimpleMath::Vector3 extents)
{
	m_extents = extents;
}

void HEIN::OBBColliderComponent::InitializeFromModel(StaticModelComponent* staticModel)
{
    if (staticModel != nullptr)
    {
        DirectX::BoundingBox box = staticModel->GetBoundingBox();

        m_extents = box.Extents;

        m_offset = box.Center;
    }
}

void HEIN::OBBColliderComponent::Update(float deltaTime)
{
}

void HEIN::OBBColliderComponent::Draw(
    GameContext& gameContext,
    const DirectX::SimpleMath::Matrix& world,
    const DirectX::SimpleMath::Matrix& view,
    const DirectX::SimpleMath::Matrix& proj
)
{
    if (gameContext.debugCollisionRenderer == nullptr) return;

    DirectX::SimpleMath::Matrix worldMatrix = CalculateWorldMatrix();

    DirectX::SimpleMath::Vector3 center = worldMatrix.Translation();

    DirectX::SimpleMath::Vector3 right(worldMatrix._11, worldMatrix._12, worldMatrix._13);
    DirectX::SimpleMath::Vector3 up(worldMatrix._21, worldMatrix._22, worldMatrix._23);
    DirectX::SimpleMath::Vector3 forward(worldMatrix._31, worldMatrix._32, worldMatrix._33);

    DirectX::SimpleMath::Vector3 scale(right.Length(), up.Length(), forward.Length());

    right.Normalize();
    up.Normalize();
    forward.Normalize();

    worldMatrix._11 = right.x; worldMatrix._12 = right.y; worldMatrix._13 = right.z;
    worldMatrix._21 = up.x;    worldMatrix._22 = up.y;    worldMatrix._23 = up.z;
    worldMatrix._31 = forward.x; worldMatrix._32 = forward.y; worldMatrix._33 = forward.z;

    DirectX::SimpleMath::Quaternion rotation = DirectX::SimpleMath::Quaternion::CreateFromRotationMatrix(worldMatrix);

    DirectX::SimpleMath::Vector3 scaledExtents = m_extents * scale;

    DirectX::BoundingOrientedBox obb(center, scaledExtents, rotation);

    DirectX::SimpleMath::Color debugColor = DirectX::SimpleMath::Color(DirectX::Colors::Red);
    if (m_isTrigger)
    {
        debugColor = DirectX::Colors::Yellow;
    }

    gameContext.debugCollisionRenderer->QueueOBB(obb, debugColor);
}
