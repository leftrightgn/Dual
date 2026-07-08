#include "pch.h" 
#include "States/CombatStates.h"
#include "Components/CombatStateMachineComponent.h"
#include <BlackBoard/CombatBlackBoard.h>
#include <Components/SkinnedModelComponent.h>
#include <Components/TransformComponent.h>

// ==============================================================================
// IDLE STATE
// ==============================================================================
HEIN::IdleState::IdleState(const HEIN::StateConfig& config) : m_config(config) {}

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

	// Only handle movement transitions in Update
	if (blackboard->moveIntent.LengthSquared() > 0.1f)
	{
		if (blackboard->isLockedOn) stateMachine->ChangeState(m_config.transitions["OnStrafe"]);
		else stateMachine->ChangeState(m_config.transitions["OnMove"]);
	}
}

bool HEIN::IdleState::HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID)
{
	switch (messageID)
	{
	case Message::PLAYER_ACTION_ATTACK: stateMachine->ChangeState(m_config.transitions["OnAttack"]); return true;
	case Message::PLAYER_ACTION_DODGE:  stateMachine->ChangeState(m_config.transitions["OnDodge"]); return true;
	case Message::PLAYER_ACTION_BLOCK:  stateMachine->ChangeState(m_config.transitions["OnBlock"]); return true;
	}
	return false;
}

void HEIN::IdleState::OnExit(Actor* /*owner*/, CombatStateMachineComponent* /*stateMachine*/) {}

// ==============================================================================
// WALK STATE
// ==============================================================================
HEIN::WalkState::WalkState(const HEIN::StateConfig& config) : m_config(config) {}

void HEIN::WalkState::OnEnter(Actor* owner, CombatStateMachineComponent* /*stateMachine*/)
{
	HEIN::CombatBlackBoard* blackboard = owner->GetComponent<CombatBlackBoard>();
	if (blackboard)
	{
		blackboard->currentStance = CombatStance::Walking;
		// blackboard->currentSpeed = m_config.moveSpeed; // Uncomment if added to blackboard
	}
	std::vector<HEIN::SkinnedModelComponent*> models = owner->GetComponents<SkinnedModelComponent>();
	for (HEIN::SkinnedModelComponent* model : models)
	{
		model->CrossfadeAnimation(m_config.animationName, 0.02f);
	}
}

void HEIN::WalkState::Update(Actor* owner, CombatStateMachineComponent* stateMachine, float /*deltaTime*/)
{
	HEIN::CombatBlackBoard* blackboard = owner->GetComponent<CombatBlackBoard>();
	if (!blackboard) return;

	if (blackboard->moveIntent.LengthSquared() <= 0.1f)
	{
		stateMachine->ChangeState(m_config.transitions["OnStop"]);
		return;
	}

	if (blackboard->isLockedOn)
	{
		stateMachine->ChangeState(m_config.transitions["OnStrafe"]);
		return;
	}
}

bool HEIN::WalkState::HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID)
{
	switch (messageID)
	{
	case Message::PLAYER_ACTION_ATTACK: stateMachine->ChangeState(m_config.transitions["OnAttack"]); return true;
	case Message::PLAYER_ACTION_DODGE:  stateMachine->ChangeState(m_config.transitions["OnDodge"]); return true;
	case Message::PLAYER_ACTION_BLOCK:  stateMachine->ChangeState(m_config.transitions["OnBlock"]); return true;
	}
	return false;
}

void HEIN::WalkState::OnExit(Actor* /*owner*/, CombatStateMachineComponent* /*stateMachine*/) {}

// ==============================================================================
// ONE HAND ATTACK STATE
// ==============================================================================
HEIN::OneHandAttackState::OneHandAttackState(const StateConfig& config) : m_config(config) {}

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
	m_comboStage = 0;
}

void HEIN::OneHandAttackState::Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime)
{
	m_timer += deltaTime;
	HEIN::CombatBlackBoard* blackboard = owner->GetComponent<CombatBlackBoard>();

	// If the animation completely finishes the current combo stage, exit state
	if (m_timer >= m_config.comboEndTimes[m_comboStage])
	{
		if (blackboard != nullptr && blackboard->moveIntent.LengthSquared() > 0.1f)
		{
			if (blackboard->isLockedOn) stateMachine->ChangeState(m_config.transitions["OnStrafe"]); 
			else stateMachine->ChangeState(m_config.transitions["OnMove"]);
		}
		else
		{
			stateMachine->ChangeState(m_config.transitions["OnStop"]);
		}
	}
}

bool HEIN::OneHandAttackState::HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID)
{
	// Process combo inputs dynamically
	if (messageID == Message::PLAYER_ACTION_ATTACK)
	{
		if (m_comboStage < m_config.comboEndTimes.size() - 1)
		{
			if (m_timer >= m_config.comboWindowStarts[m_comboStage] &&
				m_timer < m_config.comboEndTimes[m_comboStage])
			{
				m_comboStage++;
				return true; // successfully combo'd! Buffer will clear this input.
			}
		}
	}
	// Note: Returning false means the buffer will "hold onto" the dodge/block input until the attack ends!
	return false;
}

void HEIN::OneHandAttackState::OnExit(Actor* /*owner*/, CombatStateMachineComponent* /*stateMachine*/) {}

// ==============================================================================
// DODGE STATE
// ==============================================================================
HEIN::DodgeState::DodgeState(const StateConfig& config) : m_config(config) {}

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
		if (m_timer >= m_config.stateDuration)
		{
			if (blackboard->moveIntent.LengthSquared() > 0.1f)
			{
				if (blackboard->isLockedOn) stateMachine->ChangeState(m_config.transitions["OnStrafe"]);
				else stateMachine->ChangeState(m_config.transitions["OnMove"]);
			}
			else
			{
				stateMachine->ChangeState(m_config.transitions["OnStop"]);
			}
			return;
		}

		blackboard->currentSpeed = m_config.moveSpeed;
		blackboard->moveIntent = m_lockedDirection;
	}
}

bool HEIN::DodgeState::HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID)
{
	// By returning false, any attacks or dodges pressed during the dodge animation 
	// are buffered and will execute the moment the dodge finishes.
	return false;
}

void HEIN::DodgeState::OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) {}

// ==============================================================================
// STRAFE STATE
// ==============================================================================
HEIN::StrafeState::StrafeState(const StateConfig& config) : m_config(config), m_isRight(false) {}

void HEIN::StrafeState::OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine)
{
	HEIN::CombatBlackBoard* blackboard = owner->GetComponent<CombatBlackBoard>();
	if (blackboard)
	{
		blackboard->currentStance = CombatStance::Strafing;
		m_isRight = blackboard->localMoveIntent.x < 0.0f;
	}

	std::vector<HEIN::SkinnedModelComponent*> models = owner->GetComponents<SkinnedModelComponent>();
	for (HEIN::SkinnedModelComponent* model : models)
	{
		if (m_isRight) model->CrossfadeAnimation(m_config.animationName, 0.3f);
		else model->CrossfadeAnimation(m_config.secondaryAnimationName, 0.3f);
	}

}

void HEIN::StrafeState::Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime)
{
	HEIN::CombatBlackBoard* blackboard = owner->GetComponent<HEIN::CombatBlackBoard>();
	if (!blackboard) return;

	blackboard->currentSpeed = m_config.moveSpeed;
	// Drop Lock-on
	if (!blackboard->isLockedOn)
	{
		stateMachine->ChangeState(m_config.transitions["OnMove"]);
		return;
	}

	// Stop Moving
	if (blackboard->moveIntent.LengthSquared() <= 0.1f)
	{
		stateMachine->ChangeState(m_config.transitions["OnStop"]);
		return;
	}

	// Update Strafe Direction Visuals dynamically 
	bool isRight = blackboard->localMoveIntent.x < 0.0f;
	if (isRight != m_isRight)
	{
		m_isRight = isRight;
		std::vector<HEIN::SkinnedModelComponent*> models = owner->GetComponents<SkinnedModelComponent>();
		for (HEIN::SkinnedModelComponent* model : models)
		{
			if (m_isRight) model->CrossfadeAnimation(m_config.animationName, 0.3f);
			else model->CrossfadeAnimation(m_config.secondaryAnimationName, 0.3f);
		}
	}
}

bool HEIN::StrafeState::HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID)
{
	switch (messageID)
	{
	case Message::PLAYER_ACTION_ATTACK: stateMachine->ChangeState(m_config.transitions["OnAttack"]); return true;
	case Message::PLAYER_ACTION_DODGE:  stateMachine->ChangeState(m_config.transitions["OnDodge"]); return true;
	case Message::PLAYER_ACTION_BLOCK:  stateMachine->ChangeState(m_config.transitions["OnBlock"]); return true;
	}
	return false;
}

void HEIN::StrafeState::OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) {}

// ==============================================================================
// BLOCK STATE
// ==============================================================================
HEIN::BlockState::BlockState(const StateConfig& config) : m_config(config) {}

void HEIN::BlockState::OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine)
{
	HEIN::CombatBlackBoard* blackboard = owner->GetComponent<CombatBlackBoard>();
	if (blackboard)
	{
		blackboard->currentStance = CombatStance::Blocking;
	}
	std::vector<HEIN::SkinnedModelComponent*> models = owner->GetComponents<SkinnedModelComponent>();
	for (HEIN::SkinnedModelComponent* model : models)
	{
		model->CrossfadeAnimation(m_config.animationName, 0.3f);
	}
}

void HEIN::BlockState::Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime)
{
	// Note: Blocking logic typically transitions out via a "Release" event.
	// As a fallback placeholder based on your old logic, you can check movement intent.
	HEIN::CombatBlackBoard* blackboard = owner->GetComponent<HEIN::CombatBlackBoard>();
	if (blackboard)
	{
		// You will likely need to send a `PLAYER_STOP_BLOCK` message from InputManager to exit cleanly.
		// For now, it stays trapped in Block State until you add the release input check here.
	}
}

bool HEIN::BlockState::HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID)
{
	// If the player dodges while holding block, we immediately allow it
	if (messageID == Message::PLAYER_ACTION_DODGE)
	{
		stateMachine->ChangeState(m_config.transitions["OnDodge"]);
		return true;
	}
	return false;
}

void HEIN::BlockState::OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) {}