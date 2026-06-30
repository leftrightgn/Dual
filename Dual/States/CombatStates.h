#pragma once
#include "States/ICombatState.h"
#include "Entities/Actor.h"

namespace HEIN
{
	struct StateConfig
	{
		std::string animationName;

		std::unordered_map<std::string, std::string> transitions;

		float moveSpeed = 0.0f;
		float stateDuration = 1.0f;
	};

	class CombatStateMachineComponent;
	// Idle State
	class IdleState : public ICombatState
	{
	private:

		HEIN::StateConfig m_config;
	public:
		IdleState(const HEIN::StateConfig& config);

		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine) override;

		void Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime) override;

		void OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) override;
		
	};

	// Walk State
	class WalkState : public ICombatState
	{
	private:
		HEIN::StateConfig m_config;

	public:
		WalkState(const HEIN::StateConfig& config);

		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine) override;

		void Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime) override;

		void OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) override;
	};

	// OneHandSwordAttack State
	class OneHandAttackState : public ICombatState
	{
	private:
		HEIN::StateConfig m_config;
		float m_timer = 0.0f;

	public:
		OneHandAttackState(const StateConfig& config);

		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine) override;

		void Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime) override;

		void OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) override;

	};

	class DodgeState : public ICombatState
	{
	private:
		HEIN::StateConfig m_config;
		float m_timer = 0.0f;
		DirectX::SimpleMath::Vector3 m_lockedDirection;

	public:
		DodgeState(const StateConfig& config);

		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine) override;

		void Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime) override;

		void OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) override;
	};
}
