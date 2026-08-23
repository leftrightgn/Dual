#pragma once
#include "../../External/Engine/Components/IComponent.h"
#include <SimpleMath.h>
#include <string>
#include "../../External/Engine/Entities/Actor.h"

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
	/**
	 * @brief The CombatBlackBoard struct serves as a decoupled memory hub for an actor's current combat state.
	 * 
	 * WHAT KIND OF INFO IS STORED:
	 * It purely holds raw data parameters: 
	 * - Physical Reality: Velocity, ground state, target distance/direction.
	 * - Combat Stats: Health, stamina, move intents, cooldowns, current stance.
	 * 
	 * WHY IT IS SEPARATED (DECOUPLING):
	 * By separating this data from specific AI nodes, multiple distinct systems (like Behaviour Tree Nodes, 
	 * TargetTrackingComponent, and CombatStateMachine) can read and write shared context asynchronously 
	 * without directly knowing about or including headers of each other. This prevents tight coupling 
	 * between the AI brain and the physics/animation execution layer.
	 */
	struct CombatBlackBoard : public IComponent
	{
		std::string activeNodeName = "None";

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
		float currentSpeed = 30.0f;
		float currentTurnSpeed = 12.0f;

		DirectX::SimpleMath::Vector3 moveIntent = DirectX::SimpleMath::Vector3::Zero;
		DirectX::SimpleMath::Vector3 localMoveIntent = DirectX::SimpleMath::Vector3::Zero;

		// Block Stats
		float maxBlockStamina = 2.0f;
		float currentBlockStamina = 2.0f;
		float blockRecoveryRate = 0.5f;
		bool isBlockBroken = false;
		float blockCooldownTimer = 0.0f;

		// Dodge Stats
		float dodgeCooldownTimer = 0.0f;
		float maxDodgeCooldown = 5.0f;

		CombatBlackBoard(Actor* owner)
			: IComponent(owner)
		{
		}

		void Update(float /*deltaTime*/) override {} // does nothing just hold data

	};
}
