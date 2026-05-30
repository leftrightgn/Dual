#include "pch.h"
#include "CombatStateMachineComponent.h"
#include <States/ICombatState.h>
#include "IComponent.h"
#include <memory>


HEIN::CombatStateMachineComponent::CombatStateMachineComponent(Actor* owner)
	: IComponent(owner)
{
	m_idleState = std::make_unique<IdleState>();
	m_walkState = std::make_unique<WalkState>();
	m_oneHandAtkState = std::make_unique<OneHandAttackState>();
}

void HEIN::CombatStateMachineComponent::Start()
{
	ChangeState(m_idleState.get());
}

void HEIN::CombatStateMachineComponent::Update(float deltaTime)
{
	ApplyPendingState();

	if (m_currentState)
	{
		m_currentState->Update(m_owner, this, deltaTime);
	}
}

void HEIN::CombatStateMachineComponent::ChangeState(ICombatState* newState)
{
	m_pendingState = newState;
}

void HEIN::CombatStateMachineComponent::ApplyPendingState()
{
	// if there is nothing waiting do nothing and leave
	if (m_pendingState == nullptr) return;
	// safely exit the olddstate
	if (m_currentState != nullptr)
	{
		m_currentState->OnExit(m_owner, this);
	}
    // swap the state
	m_currentState = m_pendingState;

	// Empty the waiting room
	m_pendingState = nullptr;

	if (m_currentState != nullptr)
	{
		m_currentState->OnEnter(m_owner, this);
	}
}
