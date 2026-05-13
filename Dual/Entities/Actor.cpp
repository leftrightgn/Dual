#include "Actor.h"

HEIN::Actor::Actor(const std::wstring& tag)
	: m_tag(tag)
{
}

void HEIN::Actor::Update(float deltaTime)
{
	for (auto& comp : m_components)
	{
		comp->Update(deltaTime);
	}
}

void HEIN::Actor::Draw(GameContext& gameContext, const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj)
{
	TransformComponent* transform = GetComponent<TransformComponent>();
	if (!transform) return;

	DirectX::SimpleMath::Matrix world = transform->GetWorldMatrix();


	for (auto& component : m_components)
	{
		component->Draw(gameContext, world, view, proj);
	}
}
