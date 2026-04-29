#pragma once
#include <SimpleMath.h>
#include "IComponent.h"

namespace HEIN
{
    class TransformComponent : public IComponent
    {
    private:
        DirectX::SimpleMath::Vector3 m_position;
        DirectX::SimpleMath::Vector3 m_rotation;
        DirectX::SimpleMath::Vector3 m_scale;

    public:

        TransformComponent(Actor* owner)
            : IComponent(owner)
            , m_position(0.0f, 0.0f, 0.0f)
            , m_rotation(0.0f, 0.0f, 0.0f)
            , m_scale(1.0f, 1.0f, 1.0f)
        {
        }

        // --- Getters & Setters ---

        void SetPosition(const DirectX::SimpleMath::Vector3& pos) { m_position = pos; }
        const DirectX::SimpleMath::Vector3& GetPosition() const { return m_position; }

        void SetRotation(const DirectX::SimpleMath::Vector3& rot) { m_rotation = rot; }
        const DirectX::SimpleMath::Vector3& GetRotation() const { return m_rotation; }

        void SetScale(const DirectX::SimpleMath::Vector3& scale) { m_scale = scale; }
        const DirectX::SimpleMath::Vector3& GetScale() const { return m_scale; }

        // --- Core 3D Math ---

        // Generates the World Matrix for DirectX 11 rendering
        DirectX::SimpleMath::Matrix GetWorldMatrix() const
        {

            // the order of matrix multiplication is Scale * Rotation * Translation
            // CreateFromYawPitchRoll takes (Y, X, Z) 
            return  DirectX::SimpleMath::Matrix::CreateScale(m_scale) *
                DirectX::SimpleMath::Matrix::CreateFromYawPitchRoll(m_rotation.y, m_rotation.x, m_rotation.z) *
                DirectX::SimpleMath::Matrix::CreateTranslation(m_position);
        }

        void Update(float deltaTime) override {}

    };
}