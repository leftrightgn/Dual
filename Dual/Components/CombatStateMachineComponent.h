#pragma once
#include <States/CombatStates.h>
#include "Common/Event.h"
#include <Message/Message.h>

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

		std::vector<Message::MessageID> m_messageBuffer;
		static constexpr float MAX_BUFFER_TIME = 0.2f;
		float m_bufferTime = 0.0f;

	public:

		CombatStateMachineComponent(Actor* owner);
	
		void Update(float deltaTime) override;

		void ChangeState(const std::string& stateName);

		void ApplyPendingState();
		
		void AddState(const std::string& stateName, std::unique_ptr<ICombatState> state);

		void HandleMessage(Message::MessageID messageID);

		void ProcessBuffer(float deltaTime);
	private:

		void OnTriggerOverLap(const HEIN::TriggerEventPayLoad& payLoad);
	};

}

