#pragma once
#include "../../External/Engine/Components/IComponent.h"
#include <SimpleMath.h>
#include <string>
#include "../../External/Engine/Entities/Actor.h"
#include "../../External/Engine/ImGui/imgui.h"

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
		Blocking,
		Jumping
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

		// Root Motion State
		bool hasRootMotion = false;
		DirectX::SimpleMath::Vector3 rootMotionVelocity = DirectX::SimpleMath::Vector3::Zero;
		DirectX::SimpleMath::Vector3 rootMotionDelta = DirectX::SimpleMath::Vector3::Zero;

		bool isLockedOn = false;
		HEIN::ActorID lockedTargetID = HEIN::INVALID_ACTOR_ID;

		// Combat Stats
		float currentStamina = 100.0f;
		float maxStamina = 100.0f;
		float currentHealth = 100.0f;
		CombatStance currentStance = CombatStance::Idle;
		float currentSpeed = 30.0f;
		float currentTurnSpeed = 12.0f;

		bool isAttacking = false;
		bool isJumping = false;

		DirectX::SimpleMath::Vector3 moveIntent = DirectX::SimpleMath::Vector3::Zero;
		DirectX::SimpleMath::Vector3 localMoveIntent = DirectX::SimpleMath::Vector3::Zero;

		// Block Stats
		float maxBlockStamina = 1.0f;
		float currentBlockStamina = 1.0f;
		float blockRecoveryRate = 0.5f;
		bool isBlockBroken = false;
		float blockCooldownTimer = 0.0f;      // > 0 after a successful block: incoming hits are ignored
		float blockCooldownDuration = 0.8f;   // Immunity window after a successful block (covers the rest of the swing)

		// Dodge Stats
		float dodgeCooldownTimer = 0.0f;
		float maxDodgeCooldown = 3.5f;

		bool hasBeenSeenByPlayer = false;

		bool hasSetSpawnPosition = false;
		DirectX::SimpleMath::Vector3 spawnPosition = DirectX::SimpleMath::Vector3::Zero;
		float aggroRange = 80.0f;

		bool isLeashing = false;

		CombatStance targetStance = CombatStance::Idle;
		bool isTargetAttacking = false;
		float targetHealth = 100.0f;
		float actionCooldownTimer = 0.0f;

		// Hit confirmation & combo tracking
		bool lastAttackHit = false;
		int comboIndex = 0;

		CombatBlackBoard(Actor* owner)
			: IComponent(owner)
		{
		}

		void Update(float /*deltaTime*/) override {}

		void OnInspectorGUI(GameContext& /*gameContext*/) override
		{
			if (ImGui::CollapsingHeader("CombatBlackBoard", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::DragFloat("Health", &currentHealth, 0.5f, 0.0f, 1000.0f);
				ImGui::DragFloat("Stamina", &currentStamina, 0.5f, 0.0f, 1000.0f);
				ImGui::DragFloat("Speed", &currentSpeed, 0.5f, 0.0f, 200.0f);
				ImGui::DragFloat("Turn Speed", &currentTurnSpeed, 0.1f, 0.0f, 50.0f);
				ImGui::Checkbox("Is Grounded", &isGrounded);
				ImGui::Checkbox("Is Attacking", &isAttacking);
				ImGui::Checkbox("Is Jumping", &isJumping);
				ImGui::Text("Move Intent: (%.2f, %.2f, %.2f)", moveIntent.x, moveIntent.y, moveIntent.z);
				ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", currentVelocity.x, currentVelocity.y, currentVelocity.z);
			}
		}

		std::string GetComponentName() const override { return "CombatBlackBoard"; }
	};
}
