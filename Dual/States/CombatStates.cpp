#include "pch.h" 
#include "States/CombatStates.h"
#include "Components/CombatStateMachineComponent.h"
#include <BlackBoard/CombatBlackBoard.h>
#include <Components/SkinnedModelComponent.h>
#include <Components/TransformComponent.h>


HEIN::IdleState::IdleState(const HEIN::StateConfig& config)
	: m_config(config)
{
}

void HEIN::IdleState::OnEnter(Actor* owner, CombatStateMachineComponent* /*stateMachine*/)
{
	HEIN::CombatBlackBoard* blackboard = owner->GetComponent<CombatBlackBoard>();
	if (blackboard) blackboard->currentStance = CombatStance::Idle;

	std::vector<HEIN::SkinnedModelComponent*> models = owner->GetComponents<SkinnedModelComponent>();
	for (HEIN::SkinnedModelComponent* model : models)
	{
		model->CrossfadeAnimation(m_config.animationName, 0.2f);
	}
}

void HEIN::IdleState::Update(Actor* owner, CombatStateMachineComponent* stateMachine, float /*deltaTime*/)
{
	HEIN::CombatBlackBoard* blackboard = owner->GetComponent<CombatBlackBoard>();
	if (!blackboard) return;

	if (blackboard->isAttackingIntent && blackboard->currentStamina >= 15.0f) {
		stateMachine->ChangeState(m_config.transitions["OnAttack"]);
		return;
	}
	if (blackboard->moveIntent.LengthSquared() > 0.1f)
	{
		stateMachine->ChangeState(m_config.transitions["OnMove"]);
		return;
	}
	if (blackboard->isDodgingIntent)
	{
		stateMachine->ChangeState(m_config.transitions["OnDodge"]);
		return;
	}

}

void HEIN::IdleState::OnExit(Actor* /*owner*/, CombatStateMachineComponent* /*stateMachine*/)
{
}

HEIN::WalkState::WalkState(const HEIN::StateConfig& config)
	: m_config(config)
{
}

void HEIN::WalkState::OnEnter(Actor* owner, CombatStateMachineComponent* /*stateMachine*/)
{
	HEIN::CombatBlackBoard* blackboard = owner->GetComponent<CombatBlackBoard>();
	if (blackboard) blackboard->currentStance = CombatStance::Walking;

	std::vector<HEIN::SkinnedModelComponent*> models = owner->GetComponents<SkinnedModelComponent>();
	for (HEIN::SkinnedModelComponent* model : models)
	{
		model->CrossfadeAnimation(m_config.animationName, 0.05f);
	}
}

void HEIN::WalkState::Update(Actor* owner, CombatStateMachineComponent* stateMachine, float /*deltaTime*/)
{
	HEIN::CombatBlackBoard* blackboard = owner->GetComponent<CombatBlackBoard>();
	if (!blackboard) return;

	if (blackboard->isAttackingIntent ) {
		stateMachine->ChangeState(m_config.transitions["OnAttack"]);
		return;
	}

	if (blackboard->moveIntent.LengthSquared() <= 0.1f)
	{
		stateMachine->ChangeState(m_config.transitions["OnStop"]);
		return;
	}
}

void HEIN::WalkState::OnExit(Actor* /*owner*/, CombatStateMachineComponent* /*stateMachine*/)
{
}

HEIN::OneHandAttackState::OneHandAttackState(const StateConfig& config)
	: m_config(config)
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
		model->CrossfadeAnimation(m_config.animationName, 0.3f);
	}
	m_timer = 0.0f;
}

void HEIN::OneHandAttackState::Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime)
{
	m_timer += deltaTime;
	HEIN::CombatBlackBoard* blackboard = owner->GetComponent<CombatBlackBoard>();
	if (m_timer >= m_config.stateDuration)
	{
		if (blackboard != nullptr)
		{
			blackboard->isAttackingIntent = false;
		}
		if (blackboard != nullptr && blackboard->moveIntent.LengthSquared() > 0.1f)
		{
			stateMachine->ChangeState(m_config.transitions["OnMove"]);
		}
		else
		{
			stateMachine->ChangeState(m_config.transitions["OnStop"]);
		}
		m_timer = 0.0f;
		
	}
}

void HEIN::OneHandAttackState::OnExit(Actor* /*owner*/, CombatStateMachineComponent* /*stateMachine*/)
{
}

HEIN::DodgeState::DodgeState(const StateConfig& config)
	: m_config(config)
{
}

void HEIN::DodgeState::OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine)
{
	HEIN::CombatBlackBoard* blackboard = owner->GetComponent<CombatBlackBoard>();
	if (blackboard)
	{
		blackboard->currentStance = CombatStance::Dodging;
		if (blackboard->moveIntent.LengthSquared() > 0.01f)
		{
			m_lockedDirection = blackboard->moveIntent;
		}
		else
		{
			HEIN::TransformComponent* trans = owner->GetComponent<HEIN::TransformComponent>();
			float currentMathematicalYaw = trans->GetRotationEuler().y;
			float trueVisualYaw = currentMathematicalYaw - DirectX::XM_PI;
			m_lockedDirection.x = sinf(trueVisualYaw);
			m_lockedDirection.y = 0.0f;
			m_lockedDirection.z = cosf(trueVisualYaw);

			m_lockedDirection.Normalize();
		}
	}
	std::vector<HEIN::SkinnedModelComponent*> models = owner->GetComponents<SkinnedModelComponent>();
	for (HEIN::SkinnedModelComponent* model : models)
	{
		model->CrossfadeAnimation(m_config.animationName, 0.3f);
	}
	m_timer = 0.0f;
}

void HEIN::DodgeState::Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime)
{
	m_timer += deltaTime;
	HEIN::CombatBlackBoard* blackboard = owner->GetComponent<HEIN::CombatBlackBoard>();

	if (blackboard)
	{
		blackboard->currentSpeed = m_config.moveSpeed;
		blackboard->moveIntent = m_lockedDirection;

	}
	if (m_timer >= m_config.stateDuration)
	{
		if (blackboard && blackboard->moveIntent.LengthSquared() > 0.1f)
			stateMachine->ChangeState(m_config.transitions["OnMove"]);
		else
			stateMachine->ChangeState(m_config.transitions["OnStop"]);
	}
}

void HEIN::DodgeState::OnExit(Actor* owner, CombatStateMachineComponent* stateMachine)
{
}

HEIN::StrafeState::StrafeState(const StateConfig& config)
	: m_config(config)
{
}

void HEIN::StrafeState::OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine)
{
}

void HEIN::StrafeState::Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime)
{
}

void HEIN::StrafeState::OnExit(Actor* owner, CombatStateMachineComponent* stateMachine)
{
}
