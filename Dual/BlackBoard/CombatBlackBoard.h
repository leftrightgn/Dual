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

	/**
	 * @brief Base CombatBlackBoard holds ONLY the core physical locomotion, root motion,
	 * and combat stance state shared across all combat actors.
	 *
	 * Specialized into:
	 * - PlayerBlackBoard (shield guard stamina, parry window, player HUD state)
	 * - BossBlackBoard   (AI Behaviour Tree node state, tether/spawn leash, 6-permutation combo table)
	 */
	struct CombatBlackBoard : public IComponent
	{
		// Physical & Locomotion Reality
		DirectX::SimpleMath::Vector3 currentVelocity = DirectX::SimpleMath::Vector3::Zero;
		bool isGrounded = true;
		DirectX::SimpleMath::Vector3 dirToTarget = DirectX::SimpleMath::Vector3::Zero;
		float distanceToTarget = 0.0f;

		// Root Motion State
		bool hasRootMotion = false;
		DirectX::SimpleMath::Vector3 rootMotionVelocity = DirectX::SimpleMath::Vector3::Zero;
		DirectX::SimpleMath::Vector3 rootMotionDelta = DirectX::SimpleMath::Vector3::Zero;

		// Target Lock-On & Alignment
		bool isLockedOn = false;
		HEIN::ActorID lockedTargetID = HEIN::INVALID_ACTOR_ID;

		// Shared Combat & Movement Stats
		float currentStamina = 100.0f;
		float maxStamina = 100.0f;
		float currentHealth = 100.0f;
		float maxHealth = 100.0f;
		CombatStance currentStance = CombatStance::Idle;
		float currentSpeed = 55.0f;
		float currentTurnSpeed = 25.0f;

		bool isAttacking = false;
		bool isJumping = false;

		DirectX::SimpleMath::Vector3 moveIntent = DirectX::SimpleMath::Vector3::Zero;
		DirectX::SimpleMath::Vector3 localMoveIntent = DirectX::SimpleMath::Vector3::Zero;

		// Shared Dodge Cooldown
		float dodgeCooldownTimer = 0.0f;
		float maxDodgeCooldown = 0.6f;

		// Hit confirmation & anti-combo burst counter
		bool lastAttackHit = false;
		int consecutiveHitsTaken = 0;

		CombatBlackBoard(Actor* owner)
			: IComponent(owner)
		{
		}

		virtual ~CombatBlackBoard() = default;

		void Update(float /*deltaTime*/) override {}

		void OnInspectorGUI(GameContext& /*gameContext*/) override
		{
			if (ImGui::CollapsingHeader("CombatBlackBoard (Base)", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::DragFloat("Health", &currentHealth, 0.5f, 0.0f, 1000.0f);
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

#include "PlayerBlackBoard.h"
#include "BossBlackBoard.h"
