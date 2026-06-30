#pragma once
#include "States/ICombatState.h"
#include "Entities/Actor.h"

namespace HEIN
{
	struct StateConfig
	{
		std::string animationName;

		std::unordered_map<std::string, std::string> transitions;
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
		const float WINDUP_DURATION = 4.1f;
	public:
		OneHandAttackState(const StateConfig& config);

		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine) override;

		void Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime) override;

		void OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) override;

	};
}
