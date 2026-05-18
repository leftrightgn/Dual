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
	if (m_currentState)
	{
		m_currentState->Update(m_owner, this, deltaTime);
	}
}

void HEIN::CombatStateMachineComponent::ChangeState(ICombatState* newState)
{
	if (m_currentState)
	{
		m_currentState->OnExit(m_owner, this);
	}

	m_currentState = newState;

	if (m_currentState)
	{
		m_currentState->OnEnter(m_owner, this);
	}
}
