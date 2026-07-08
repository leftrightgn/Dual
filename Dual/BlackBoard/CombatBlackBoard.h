#pragma once
#include "Components/IComponent.h"
#include <SimpleMath.h>
#include <Entities/Actor.h>

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
		Staggered,
		Dodging,
		Strafing,
		Blocking
	};

	// Only holds Data contains zero logic
	struct CombatBlackBoard : public IComponent
	{
		// Intentions 
		DirectX::SimpleMath::Vector3 moveIntent = DirectX::SimpleMath::Vector3::Zero;
		float currentSpeed = 30.0f;
		bool isAttackingIntent = false;
		bool isParryingIntent = false;
		bool isDodgingIntent = false;
		bool isStrafingIntent = false;
		bool lockOnIntent = false;
		bool isBlockingIntent = false;

		// Physical Reality
		DirectX::SimpleMath::Vector3 currentVelocity = DirectX::SimpleMath::Vector3::Zero;
		bool isGrounded = true;
		DirectX::SimpleMath::Vector3 dirToTarget = DirectX::SimpleMath::Vector3::Zero;
		float distanceToTarget = 0.0f;

		bool isLockedOn = false;
		HEIN::ActorID lockedTargetID = HEIN::INVALID_ACTOR_ID;

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
