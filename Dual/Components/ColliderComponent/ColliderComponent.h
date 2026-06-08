#pragma once
#include "Components/IComponent.h"
#include <string>

namespace HEIN
{
	enum class ColliderShape
	{
		Sphere,
		AABB,
		OBB,
		Capsule
	};

	class SkinnedModelComponent;

	class TransformComponent;

	class ColliderComponent : public IComponent
	{
	protected:
		
		DirectX::SimpleMath::Vector3 m_offset;
		DirectX::SimpleMath::Vector3 m_rotationEuler;
		DirectX::SimpleMath::Quaternion m_rotationOffset;
		ColliderShape m_shape;
		bool m_isTrigger;

		TransformComponent* m_transform = nullptr;
		
		DirectX::SimpleMath::Matrix m_manualMatrix;
		bool m_useManualMatrix = false;

	public:

		ColliderComponent(Actor* owner, ColliderShape shape);
		virtual ~ColliderComponent() = default;

		void Start() override;
		virtual void Update(float deltaTime) override = 0;
		virtual void Draw(
			GameContext& gameContext,
			const DirectX::SimpleMath::Matrix& world,
			const DirectX::SimpleMath::Matrix& view,
			const DirectX::SimpleMath::Matrix& proj
		) override = 0;

		void SetManualMatrix(const DirectX::SimpleMath::Matrix& matirx)
		{
			m_manualMatrix = matirx;
			m_useManualMatrix = true;
		}

		ColliderShape GetShape() { return m_shape; }
		void SetTrigger(bool active) { m_isTrigger = active; }
		bool IsTrigger() const { return m_isTrigger; }
		void SetOffset(const DirectX::SimpleMath::Vector3& offset) { m_offset = offset; }
		DirectX::SimpleMath::Vector3 GetOffset() const { return m_offset; }
		void SetRotationOffset(const DirectX::SimpleMath::Vector3& rotation)
		{
			m_rotationEuler = rotation;
			m_rotationOffset = DirectX::SimpleMath::Quaternion::CreateFromYawPitchRoll(rotation.y, rotation.x, rotation.z);
		}
		DirectX::SimpleMath::Vector3 GetRotationOffset() const { return m_rotationEuler; }
	protected:
		DirectX::SimpleMath::Matrix CalculateWorldMatrix();
	};
}
