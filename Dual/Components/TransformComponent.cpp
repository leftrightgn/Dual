#include "pch.h"
#include "TransformComponent.h"

HEIN::TransformComponent::TransformComponent(Actor* owner)
    : IComponent(owner)
    , m_position(0.0f, 0.0f, 0.0f)
    , m_rotation(0.0f, 0.0f, 0.0f)
    , m_scale(1.0f, 1.0f, 1.0f)
    , m_parentMatrix(DirectX::SimpleMath::Matrix::Identity)
{
}

DirectX::SimpleMath::Matrix HEIN::TransformComponent::GetWorldMatrix() const
{

    // the order of matrix multiplication is Scale * Rotation * Translation
    // CreateFromYawPitchRoll takes (Y, X, Z) 
    return  DirectX::SimpleMath::Matrix::CreateScale(m_scale) *
        DirectX::SimpleMath::Matrix::CreateFromYawPitchRoll(m_rotation.y, m_rotation.x, m_rotation.z) *
        DirectX::SimpleMath::Matrix::CreateTranslation(m_position) *
        m_parentMatrix;;
}

void HEIN::TransformComponent::Update(float /*deltaTime*/)
{
}
