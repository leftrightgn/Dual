#include "pch.h"
#include "DamageDealerComponent.h"

HEIN::DamageDealerComponent::DamageDealerComponent(Actor* owner)
	: IComponent(owner)
	, m_damageAmount()
	, m_damageType()
{
}

void HEIN::DamageDealerComponent::Initialize(float damageAmount, DamageType damageType)
{
	m_damageAmount = damageAmount;
	m_damageType = damageType;
}
