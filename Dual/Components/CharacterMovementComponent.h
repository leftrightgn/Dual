#pragma once
#include "Components/IComponent.h"
#include "Components/TransformComponent.h"
#include "BlackBoard/CombatBlackBoard.h"
#include "Entities/Actor.h"

namespace HEIN
{
	class CharacterMovementComponent : public HEIN::IComponent
	{
	private:

		HEIN::CombatBlackBoard* m_blackboard;
		HEIN::TransformComponent* m_transform;
		float m_acceleration = 40.0f;
		float m_friction = 8.0f;

	public:
		CharacterMovementComponent(Actor* owner)
			: IComponent(owner)
			, m_blackboard()
			, m_transform()
			, m_acceleration(40.0f)
			, m_friction(8.0f)
		{
		}


		void Start()
		{
			m_blackboard = m_owner->GetComponent<HEIN::CombatBlackBoard>();
			m_transform = m_owner->GetComponent<HEIN::TransformComponent>();
		}

		void Update(float deltaTime) override
		{
			if (!m_transform || !m_blackboard) return;

			if (m_blackboard->currentStance == HEIN::CombatStance::AttackRelese ||
				m_blackboard->currentStance == HEIN::CombatStance::Staggered)
			{
				m_blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;
			}

			// Apply physics momentum directly 
			DirectX::SimpleMath::Vector3 desiredVelocity = m_blackboard->moveIntent * m_acceleration;

			m_blackboard->currentVelocity = DirectX::SimpleMath::Vector3::Lerp(
				m_blackboard->currentVelocity,
				desiredVelocity,
				deltaTime * m_friction
			);

			// Move the actor
			DirectX::SimpleMath::Vector3 pos = m_transform->GetPosition();
			pos += m_blackboard->currentVelocity * deltaTime;
			m_transform->SetPosition(pos);
		}
	};
}