#pragma once
#include "Common/Event.h"
#include "Components/IComponent.h"
#include "Message/IObserver.h"
#include "Message/Message.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>

namespace HEIN
{
	class Actor;
	class ICombatState;
	class CombatStateMachineComponent : public IComponent, public IObserver
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
	
		void Start() override;

		void Update(float deltaTime) override;

		void ChangeState(const std::string& stateName);

		void ApplyPendingState();
		
		void AddState(const std::string& stateName, std::unique_ptr<ICombatState> state);

		void OnMessageAccepted(Message::MessageID messageID) override;

		void ProcessBuffer(float deltaTime);
	private:

		void OnTriggerOverLap(const HEIN::TriggerEventPayLoad& payLoad);
	};

}

