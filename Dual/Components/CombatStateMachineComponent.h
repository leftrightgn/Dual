#pragma once
#include <States/CombatStates.h>
#include "Common/Event.h"

namespace HEIN
{
	class ICombatState;
	class IComponent;
	class CombatStateMachineComponent : public IComponent
	{
	private:

		ICombatState* m_currentState = nullptr;
		ICombatState* m_pendingState = nullptr;

		std::unordered_map<std::string, std::unique_ptr<ICombatState>> m_states;

	public:

		CombatStateMachineComponent(Actor* owner);
	
		void Update(float deltaTime) override;

		void ChangeState(const std::string& stateName);

		void ApplyPendingState();
		
		void AddState(const std::string& stateName, std::unique_ptr<ICombatState> state);

	private:

		void OnTriggerOverLap(const HEIN::TriggerEventPayLoad& payLoad);
	};

}

