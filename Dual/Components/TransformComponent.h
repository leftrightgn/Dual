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
        DirectX::SimpleMath::Matrix m_parentMatrix;
    public:

        TransformComponent(Actor* owner);
            

        // --- Getters & Setters ---

        void SetPosition(const DirectX::SimpleMath::Vector3& pos) { m_position = pos; }
        const DirectX::SimpleMath::Vector3& GetPosition() const { return m_position; }

        void SetRotation(const DirectX::SimpleMath::Vector3& rot) { m_rotation = rot; }
        const DirectX::SimpleMath::Vector3& GetRotation() const { return m_rotation; }

        void SetScale(const DirectX::SimpleMath::Vector3& scale) { m_scale = scale; }
        const DirectX::SimpleMath::Vector3& GetScale() const { return m_scale; }

        void SetParentMatrix(const DirectX::SimpleMath::Matrix& parent) { m_parentMatrix = parent; }

        // --- Core 3D Math ---

        // Generates the World Matrix for DirectX 11 rendering
        DirectX::SimpleMath::Matrix GetWorldMatrix() const;
      
        void Update(float deltaTime) override;

    };
}