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
}
