#pragma once
#include <SimpleMath.h>
#include "IComponent.h"

class TransformComponent : public IComponent
{
private:
	DirectX::SimpleMath::Vector3 m_position;
	DirectX::SimpleMath::Vector3 m_rotation;
	DirectX::SimpleMath::Vector3 m_scale;

public:

	TransformComponent(Actor* owner)
		: IComponent(owner)
		, m_position(0.0f, 0.0f, 0.0f)
		, m_rotation(0.0f, 0.0f, 0.0f)
		, m_scale(1.0f, 1.0f, 1.0f)
	{
	}

	void SetPosition(const DirectX::SimpleMath::Vector3& pos) { m_position = pos; }
	const DirectX::SimpleMath::Vector3& GetPosition() const { return m_position; }
};