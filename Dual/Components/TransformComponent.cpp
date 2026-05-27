#include "pch.h"
#include "TransformComponent.h"

HEIN::TransformComponent::TransformComponent(Actor* owner)
    : IComponent(owner)
    , m_position(0.0f, 0.0f, 0.0f)
    , m_rotation(DirectX::SimpleMath::Quaternion::Identity)
    , m_scale(1.0f, 1.0f, 1.0f)
    , m_parentMatrix(DirectX::SimpleMath::Matrix::Identity)
{
}

void HEIN::TransformComponent::SetRotationEuler(const DirectX::SimpleMath::Vector3& eulerAngles)
{
    m_rotation = DirectX::SimpleMath::Quaternion::CreateFromYawPitchRoll(eulerAngles.y, eulerAngles.x, eulerAngles.z);
}

DirectX::SimpleMath::Matrix HEIN::TransformComponent::GetWorldMatrix() const
{

    // the order of matrix multiplication is Scale * Rotation * Translation
    // CreateFromYawPitchRoll takes (Y, X, Z) 
    return  DirectX::SimpleMath::Matrix::CreateScale(m_scale) *
        DirectX::SimpleMath::Matrix::CreateFromQuaternion(m_rotation) *
        DirectX::SimpleMath::Matrix::CreateTranslation(m_position) *
        m_parentMatrix;;
}

void HEIN::TransformComponent::Update(float /*deltaTime*/)
{
}
