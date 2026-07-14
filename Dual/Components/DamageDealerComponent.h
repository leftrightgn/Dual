#pragma once
#include <Components/IComponent.h>

namespace HEIN
{
	enum DamageType
	{
		Physical,
		Magical,
		Fire,
		Posion
	};

	class DamageDealerComponent : public IComponent
	{
	private:

		float m_damageAmount;
		DamageType m_damageType;

	public:

		DamageDealerComponent(Actor* owner);

		void Initialize(float damageAmount, DamageType damageType = DamageType::Physical);

		void Start() override{}
		void Update(float /*deltaTime*/) override{}
		
		float GetDamageAmount() const { return m_damageAmount; }
		DamageType GetDamageType() const { return m_damageType; }
	};
}
