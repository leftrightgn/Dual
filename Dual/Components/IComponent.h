#pragma once

class Actor;

class IComponent
{
protected:

	Actor* m_owner;

public:

	IComponent(Actor* owner)
		: m_owner(owner)
	{
	}

	virtual ~IComponent() = default;

	virtual void Update(float deltaTime) = 0;
	virtual void Render() = 0;

	Actor* GetOwner() const { return m_owner; }
};
