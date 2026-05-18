#pragma once
#include <States/CombatStates.h>

namespace HEIN
{
	class ICombatState;
	class IComponent;
	class CombatStateMachineComponent : public IComponent
	{
	private:

		ICombatState* m_currentState = nullptr;

		std::unique_ptr<IdleState> m_idleState;
		std::unique_ptr<WalkState> m_walkState;
		std::unique_ptr<OneHandAttackState> m_oneHandAtkState;

	public:

		CombatStateMachineComponent(Actor* owner);
		
		void Start();
	
		void Update(float deltaTime) override;

		void ChangeState(ICombatState* newState);
		
		// Getter for transitions
		IdleState* GetIdleState() { return m_idleState.get(); }
		WalkState* GetWalkState() { return m_walkState.get(); }
		OneHandAttackState* GetOneHandAtkState() { return m_oneHandAtkState.get(); }

	};

}

