#pragma once

namespace HEIN
{
	class CombatStateMachineComponent;
	class Actor;

	class ICombatState
	{
	public:

		virtual ~ICombatState() = default;

		virtual void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine) = 0;

		virtual void Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime) = 0;

		virtual void OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) = 0;
	};
}
