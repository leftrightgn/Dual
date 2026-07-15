#include "pch.h"
#include "CombatStateMachineComponent.h"
#include <States/ICombatState.h>
#include <Entities/Actor.h>
#include <Components/ColliderComponent/ColliderComponent.h>
#include "IComponent.h"
#include <memory>
#include <Message/Messenger.h>
#include <BlackBoard/CombatBlackBoard.h>


HEIN::CombatStateMachineComponent::CombatStateMachineComponent(Actor* owner)
	: IComponent(owner)
{
}

void HEIN::CombatStateMachineComponent::Start()
{
	Messenger::GetInstance()->Register(m_owner->GetID(), this);
}


void HEIN::CombatStateMachineComponent::Update(float deltaTime)
{
	ApplyPendingState();

	ProcessBuffer(deltaTime);

	if (m_pendingState != nullptr) return;

	if (m_currentState)
	{
		m_currentState->Update(m_owner, this, deltaTime);
	}

	HEIN::CombatBlackBoard* blackboard = m_owner->GetComponent<CombatBlackBoard>();
	if (blackboard)
	{
		if (!IsBlocking())
		{
			if (blackboard->currentBlockStamina < blackboard->maxBlockStamina)
			{
				blackboard->currentBlockStamina += blackboard->blockRecoveryRate * deltaTime;
				if (blackboard->currentBlockStamina >= blackboard->maxBlockStamina)
				{
					blackboard->currentBlockStamina = blackboard->maxBlockStamina;
					blackboard->isBlockBroken = false;
				}
			}
		}

		if (blackboard->dodgeCooldownTimer > 0.0f)
		{
			blackboard->dodgeCooldownTimer -= deltaTime;
			if (blackboard->dodgeCooldownTimer < 0.0f)
			{
				blackboard->dodgeCooldownTimer = 0.0f;
			}
		}
	}
}

void HEIN::CombatStateMachineComponent::ChangeState(const std::string& stateName)
{
	std::unordered_map<std::string, std::unique_ptr<ICombatState>>::iterator it = m_states.find(stateName);

	if (it != m_states.end())
	{
		m_pendingState = it->second.get();
	}
}


void HEIN::CombatStateMachineComponent::ApplyPendingState()
{
	// if there is nothing waiting do nothing and leave
	if (m_pendingState == nullptr) return;
	// safely exit the oldstate
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

void HEIN::CombatStateMachineComponent::AddState(const std::string& stateName, std::unique_ptr<ICombatState> state)
{
	m_states[stateName] = std::move(state);

	if (m_currentState == nullptr && m_pendingState == nullptr)
	{
		ChangeState(stateName);
	}
}

void HEIN::CombatStateMachineComponent::OnMessageAccepted(Message::MessageID messageID)
{
	if (messageID == Message::PLAYER_ACTION_ATTACK ||
		messageID == Message::PLAYER_ACTION_BLOCK ||
		messageID == Message::PLAYER_STOP_BLOCK ||
		messageID == Message::PLAYER_ACTION_DODGE)
	{
		if (messageID == Message::PLAYER_ACTION_DODGE)
		{
			HEIN::CombatBlackBoard* blackboard = m_owner->GetComponent<CombatBlackBoard>();
			if (blackboard && blackboard->dodgeCooldownTimer > 0.0f)
			{
				return;
			}
		}

		m_messageBuffer.clear();

		m_messageBuffer.push_back(messageID);

		m_bufferTime = MAX_BUFFER_TIME;
	}
}

void HEIN::CombatStateMachineComponent::ProcessBuffer(float deltaTime)
{
	if (m_messageBuffer.empty()) return;

	m_bufferTime -= deltaTime;

	if (m_bufferTime <= 0.0f)
	{
		m_messageBuffer.clear();
		return;
	}

	Message::MessageID currentMessage = m_messageBuffer.front();

	if (m_currentState != nullptr)
	{
		bool wasHandled = m_currentState->HandleMessage(m_owner, this, currentMessage);
		if (wasHandled)
		{
			m_messageBuffer.clear();
		}
	}

}

bool HEIN::CombatStateMachineComponent::IsAttacking() const
{
	if (m_currentState != nullptr)
	{
		return m_currentState->IsAttackState();
	}
	return false;
}

bool HEIN::CombatStateMachineComponent::IsBlocking() const
{
	if (m_currentState != nullptr)
	{
		return m_currentState->IsBlockState();
	}
	return false;
}

void HEIN::CombatStateMachineComponent::OnTriggerOverLap(const HEIN::TriggerEventPayLoad& payLoad)
{
	if (payLoad.triggerA->GetOwner() != GetOwner() && payLoad.triggerB->GetOwner() != GetOwner())
	{
		return;
	}

	HEIN::ColliderComponent* myHurtBox = nullptr;
	HEIN::ColliderComponent* enemyHitBox = nullptr;

	if (payLoad.triggerA->GetOwner() == GetOwner())
	{
		myHurtBox = payLoad.triggerA;
		enemyHitBox = payLoad.triggerB;
	}
	else
	{
		myHurtBox = payLoad.triggerB;
		enemyHitBox = payLoad.triggerA;
	}

	if (enemyHitBox->GetColliderTag() == L"SwordHitbox")
	{
		// Process damage, change to flinch state, subtract HP!
	}
}

