#pragma once
#include "States/ICombatState.h"
#include "Entities/Actor.h"
#include <string>
#include <unordered_map>
#include <vector>
namespace HEIN
{
	struct StateConfig
	{
		std::string animationName;
		std::string secondaryAnimationName;

		std::unordered_map<std::string, std::string> transitions;

		std::vector<float> comboEndTimes;
		std::vector<float> comboWindowStarts;

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

		bool HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID) override;
		
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

		bool HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID) override;
	};

	// OneHandSwordAttack State
	class OneHandAttackState : public ICombatState
	{
	private:
		HEIN::StateConfig m_config;
		float m_timer = 0.0f;
		int m_comboStage = 0;

	public:
		OneHandAttackState(const StateConfig& config);

		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine) override;

		void Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime) override;

		void OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) override;

		bool HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID) override;

		bool IsAttackState() const override { return true; }
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

		bool HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID) override;
	};

	class StrafeState : public ICombatState
	{
	private:

		HEIN::StateConfig m_config;
	
		bool m_isRight;
	public:

		StrafeState(const StateConfig& config);

		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine) override;

		void Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime) override;

		void OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) override;

		bool HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID) override;
	};

	class BlockState : public ICombatState
	{
	private:

		HEIN::StateConfig m_config;

	public:

		BlockState(const StateConfig& config);

		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine) override;

		void Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime) override;

		void OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) override;

		bool HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID) override;

		bool IsBlockState() const override { return true; }
	};
}
