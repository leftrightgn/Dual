#pragma once
#include "Components/IComponent.h"

namespace HEIN
{
	struct CombatBlackBoard;
	class TransformComponent;
	class Actor;

	class CharacterMovementComponent : public HEIN::IComponent
	{
	private:

		static constexpr float DEFAULT_FRICTION = 8.0f;

	private:

		HEIN::CombatBlackBoard* m_blackboard;
		HEIN::TransformComponent* m_transform;
		float m_friction;

	public:
		CharacterMovementComponent(Actor* owner);
	
		void Start() override;
	
		void Update(float deltaTime) override;
		
	};
}