#pragma once
#include "States/ICombatState.h"
#include "Entities/Actor.h"

namespace HEIN
{
	class CombatStateMachineComponent;
	// Idle State
	class IdleState : public ICombatState
	{
	public:
		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine) override;

		void Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime) override;

		void OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) override;
		
	};

	// Walk State
	class WalkState : public ICombatState
	{
	public:
		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine) override;

		void Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime) override;

		void OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) override;
	};

	// OneHandSwordAttack State
	class OneHandAttackState : public ICombatState
	{
	private:
		float m_timer = 0.0f;
		const float WINDUP_DURATION = 4.5f;
	public:
		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine) override;

		void Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime) override;

		void OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) override;

	};
}
