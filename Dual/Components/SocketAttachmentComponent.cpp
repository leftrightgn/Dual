#include "pch.h"
#include "SocketAttachmentComponent.h"
#include "SocketComponent.h"
#include <Entities/Actor.h>
#include <Components/TransformComponent.h>

HEIN::SocketAttachmentComponent::SocketAttachmentComponent(Actor* owner)
	: IComponent(owner)
	, m_targetSocket(nullptr)
	, m_socketName(L"")
{
}

void HEIN::SocketAttachmentComponent::Initialize(SocketComponent* targetSocket, const std::wstring& socketName)
{
	m_targetSocket = targetSocket;
	m_socketName = socketName;
}

void HEIN::SocketAttachmentComponent::Update(float deltaTime)
{
	if (m_targetSocket == nullptr) return;

	if (m_targetSocket->HasSocket(m_socketName))
	{
		DirectX::SimpleMath::Matrix socketWorld = m_targetSocket->GetSocketWorldMatrix(m_socketName);

		HEIN::TransformComponent* myTransform = m_owner->GetComponent<HEIN::TransformComponent>();

		if (myTransform != nullptr)
		{
			myTransform->SetParentMatrix(socketWorld);
		}
	}
}
