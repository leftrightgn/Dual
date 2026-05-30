#pragma once
#include "Components/IComponent.h"
#include <SimpleMath.h>

namespace HEIN
{
	enum class CombatStance
	{
		Idle,
		Walking,
		AttackWindup,
		OneHand,
		AttackRelese,
		AttackRecovery,
		Parrying,
		Staggered
	};

	// Only holds Data contains zero logic
	struct CombatBlackBoard : public IComponent
	{
		// Intentions 
		DirectX::SimpleMath::Vector3 moveIntent = DirectX::SimpleMath::Vector3::Zero;
		bool isAttackingIntent = false;
		bool isParryingIntent = false;

		// Physical Reality
		DirectX::SimpleMath::Vector3 currentVelocity = DirectX::SimpleMath::Vector3::Zero;
		bool isGrounded = true;

		// Combat Stats
		float currentStamina = 100.0f;
		float maxStamina = 100.0f;
		float currentHealth = 100.0f;
		CombatStance currentStance = CombatStance::Idle;

		CombatBlackBoard(Actor* owner)
			: IComponent(owner)
		{
		}

		void Update(float /*deltaTime*/) override {} // does nothing just hold data

	};
}
