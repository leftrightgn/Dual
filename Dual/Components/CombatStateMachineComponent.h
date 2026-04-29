#pragma once
#include <memory>
#include "Components/IComponent.h"
#include "States/CombatStates.h"

namespace HEIN
{
	class CombatStateMachineComponent : public IComponent
	{
	private:

		ICombatState* m_currentState = nullptr;

		std::unique_ptr<IdleState> m_idleState;
		std::unique_ptr<WalkState> m_walkState;

	public:

		CombatStateMachineComponent(Actor* owner)
			: IComponent(owner)
		{
			m_idleState = std::make_unique<IdleState>();
			m_walkState = std::make_unique<WalkState>();
		}

		void Start()
		{
			ChangeState(m_idleState.get());
		}

		void Update(float deltaTime) override
		{
			if (m_currentState)
			{
				m_currentState->Update(m_owner, this, deltaTime);
			}
		}

		void ChangeState(ICombatState* newState)
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

		// Getter for transitions
		IdleState* GetIdleState() { return m_idleState.get(); }
		WalkState* GetWalkState() { return m_walkState.get(); }

	};

	inline void IdleState::Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime)
	{
		auto blackboard = owner->GetComponent<CombatBlackBoard>();
		if (!blackboard) return;

		if (blackboard->moveIntent.LengthSquared() > 0.1f)
		{
			stateMachine->ChangeState(stateMachine->GetWalkState());
			return;
		}
	}

	inline void WalkState::Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime)
	{
		auto blackboard = owner->GetComponent<CombatBlackBoard>();
		if (!blackboard) return;

		if (blackboard->moveIntent.LengthSquared() <= 0.1f)
		{
			stateMachine->ChangeState(stateMachine->GetIdleState());
			return;
		}

	}
}

