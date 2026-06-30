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

DirectX::SimpleMath::Vector3 HEIN::TransformComponent::GetRotationEuler() const
{
   DirectX::SimpleMath::Vector3 euler;

   // Extract pitch(x-axis)
   //x=arcsin(2(𝑤⋅𝑦−𝑧⋅𝑥) 
   float sinp = 2.0f * (m_rotation.w * m_rotation.x - m_rotation.y * m_rotation.z);
   if (std::abs(sinp) >= 1.0f)
   {
       euler.x = std::copysign(DirectX::XM_PIDIV2, sinp);
   }
   else
   {
       euler.x = std::asin(sinp);
   }

   // Extract yaw(y-axis)
   //y=atan2(2(w*x+y*z),1-2(x*x+y*y)
   float siny_cosp = 2.0f * (m_rotation.w * m_rotation.y + m_rotation.z * m_rotation.x);
   float cosy_cosp = 1.0f - 2.0f * (m_rotation.x * m_rotation.x + m_rotation.y * m_rotation.y);
   euler.y = std::atan2(siny_cosp, cosy_cosp);

   // Extract Roll(z-axis)
   //z=atan2(2(w*z+x*y),1-2(y*y+z*z))
   float sinr_cosp = 2.0f * (m_rotation.w * m_rotation.z + m_rotation.x * m_rotation.y);
   float cosr_cosp = 1.0f - 2.0f * (m_rotation.y * m_rotation.y + m_rotation.z * m_rotation.z);
   euler.z = std::atan2(sinr_cosp, cosr_cosp);

   return euler;
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
