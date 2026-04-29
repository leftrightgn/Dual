#include "pch.h" 
#include "States/CombatStates.h"
#include "Components/CombatStateMachineComponent.h"
#include <BlackBoard/CombatBlackBoard.h>
#include <Components/SkinnedModelComponent.h>

void HEIN::IdleState::OnEnter(Actor* owner, CombatStateMachineComponent* /*stateMachine*/)
{
	auto blackboard = owner->GetComponent<CombatBlackBoard>();
	if (blackboard) blackboard->currentStance = CombatStance::Idle;

	auto models = owner->GetComponents<SkinnedModelComponent>();
	for (auto* model : models)
	{
		model->ChangeAnimation("Idle");
	}
}

void HEIN::IdleState::Update(Actor* owner, CombatStateMachineComponent* stateMachine, float /*deltaTime*/)
{
	auto blackboard = owner->GetComponent<CombatBlackBoard>();
	if (!blackboard) return;

	if (blackboard->moveIntent.LengthSquared() > 0.1f)
	{
		stateMachine->ChangeState(stateMachine->GetWalkState());
		return;
	}
}

void HEIN::IdleState::OnExit(Actor* /*owner*/, CombatStateMachineComponent* /*stateMachine*/)
{
}

void HEIN::WalkState::OnEnter(Actor* owner, CombatStateMachineComponent* /*stateMachine*/)
{
	auto blackboard = owner->GetComponent<CombatBlackBoard>();
	if (blackboard) blackboard->currentStance = CombatStance::Walking;

	auto models = owner->GetComponents<SkinnedModelComponent>();
	for (auto* model : models)
	{
		model->ChangeAnimation("Walk");
	}
}

void HEIN::WalkState::Update(Actor* owner, CombatStateMachineComponent* stateMachine, float /*deltaTime*/)
{
	auto blackboard = owner->GetComponent<CombatBlackBoard>();
	if (!blackboard) return;

	if (blackboard->moveIntent.LengthSquared() <= 0.1f)
	{
		stateMachine->ChangeState(stateMachine->GetIdleState());
		return;
	}
}

void HEIN::WalkState::OnExit(Actor* /*owner*/, CombatStateMachineComponent* /*stateMachine*/)
{
}
