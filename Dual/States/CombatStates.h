#pragma once
#include "BlackBoard/CombatBlackBoard.h"
#include "States/ICombatState.h"
#include "Components/SkinnedModelComponent.h"
#include "Entities/Actor.h"

namespace HEIN
{
	// Idle State
	class IdleState : public ICombatState
	{
	public:
		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine) override
		{
			auto blackboard = owner->GetComponent<CombatBlackBoard>();
			auto model = owner->GetComponent<SkinnedModelComponent>();

			if (blackboard) blackboard->currentStance = CombatStance::Idle;

			if (model) model->ChangeAnimation("Idle");
		}

		void Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime) override {}
		void OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) override {}
	};

	// Walk State
	class WalkState : public ICombatState
	{
	public:
		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine) override
		{
			auto blackboard = owner->GetComponent<CombatBlackBoard>();
			auto model = owner->GetComponent<SkinnedModelComponent>();

			if (blackboard) blackboard->currentStance = CombatStance::Walking;

			if (model) model->ChangeAnimation("Walk");
		}

		void Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime) override {}
		void OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) override {}
	};
}
