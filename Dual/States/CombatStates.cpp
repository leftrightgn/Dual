#include "pch.h" 
#include "States/CombatStates.h"
#include "Components/CombatStateMachineComponent.h"
#include <BlackBoard/CombatBlackBoard.h>
#include <Components/SkinnedModelComponent.h>

void HEIN::IdleState::OnEnter(Actor* owner, CombatStateMachineComponent* /*stateMachine*/)
{
	HEIN::CombatBlackBoard* blackboard = owner->GetComponent<CombatBlackBoard>();
	if (blackboard) blackboard->currentStance = CombatStance::Idle;

	std::vector<HEIN::SkinnedModelComponent*> models = owner->GetComponents<SkinnedModelComponent>();
	for (HEIN::SkinnedModelComponent* model : models)
	{
		model->CrossfadeAnimation("Idle", 0.2f);
	}
}

void HEIN::IdleState::Update(Actor* owner, CombatStateMachineComponent* stateMachine, float /*deltaTime*/)
{
	HEIN::CombatBlackBoard* blackboard = owner->GetComponent<CombatBlackBoard>();
	if (!blackboard) return;

	if (blackboard->isAttackingIntent && blackboard->currentStamina >= 15.0f) {
		stateMachine->ChangeState(stateMachine->GetOneHandAtkState());
		return;
	}
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
	HEIN::CombatBlackBoard* blackboard = owner->GetComponent<CombatBlackBoard>();
	if (blackboard) blackboard->currentStance = CombatStance::Walking;

	std::vector<HEIN::SkinnedModelComponent*> models = owner->GetComponents<SkinnedModelComponent>();
	for (HEIN::SkinnedModelComponent* model : models)
	{
		model->CrossfadeAnimation("Walk", 0.05f);
	}
}

void HEIN::WalkState::Update(Actor* owner, CombatStateMachineComponent* stateMachine, float /*deltaTime*/)
{
	HEIN::CombatBlackBoard* blackboard = owner->GetComponent<CombatBlackBoard>();
	if (!blackboard) return;

	if (blackboard->isAttackingIntent ) {
		stateMachine->ChangeState(stateMachine->GetOneHandAtkState());
		return;
	}

	if (blackboard->moveIntent.LengthSquared() <= 0.1f)
	{
		stateMachine->ChangeState(stateMachine->GetIdleState());
		return;
	}
}

void HEIN::WalkState::OnExit(Actor* /*owner*/, CombatStateMachineComponent* /*stateMachine*/)
{
}

void HEIN::OneHandAttackState::OnEnter(Actor* owner, CombatStateMachineComponent* /*stateMachine*/)
{
	HEIN::CombatBlackBoard* blackboard = owner->GetComponent<CombatBlackBoard>();
	if (blackboard)
	{
		blackboard->currentStance = CombatStance::OneHand;
	}
	std::vector<HEIN::SkinnedModelComponent*> models = owner->GetComponents<SkinnedModelComponent>();
	for (HEIN::SkinnedModelComponent* model : models)
	{
		model->CrossfadeAnimation("OneHand", 0.3f);
	}
	m_timer = 0.0f;
}

void HEIN::OneHandAttackState::Update(Actor* /*owner*/, CombatStateMachineComponent* stateMachine, float deltaTime)
{
	m_timer += deltaTime;

	if (m_timer >= WINDUP_DURATION)
	{
		stateMachine->ChangeState(stateMachine->GetIdleState());
		m_timer = 0.0f;
	}
}

void HEIN::OneHandAttackState::OnExit(Actor* /*owner*/, CombatStateMachineComponent* /*stateMachine*/)
{
}
