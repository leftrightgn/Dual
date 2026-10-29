#pragma once
#include "SimpleMath.h"
#include "../../External/Engine/Components/IComponent.h"
#include "../../External/Engine/Entities/Actor.h"
#if HEIN_WITH_EDITOR
#include "../../External/Engine/ImGui/imgui.h"
#endif
#include <string>

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
		bool useStamina = false;
		float currentStamina = 100.0f;
		float baseStaminaRecoveryRate = 40.0f;
		float baseStaminaRegenDelay = 0.5f;
		float baseStaminaRegenTimer = 0.0f;
		float minStaminaFloor = -30.0f;
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
		DirectX::SimpleMath::Vector3 lookIntent = DirectX::SimpleMath::Vector3::Zero;

		// Shared Dodge Cooldown
		float dodgeCooldownTimer = 0.0f;
		float maxDodgeCooldown = 0.6f;

		// Hit confirmation & anti-combo burst counter
		bool lastAttackHit = false;
		int consecutiveHitsTaken = 0;

		// AI Perception & Target Awareness
		std::string activeNodeName = "None";
		CombatStance targetStance = CombatStance::Idle;
		float targetHealth = 100.0f;
		bool isTargetAttacking = false;

		// Territorial Leashing & Bounds
		bool isLeashing = false;
		bool hasSetSpawnPosition = false;
		DirectX::SimpleMath::Vector3 spawnPosition = DirectX::SimpleMath::Vector3::Zero;
		float aggroRange = 80.0f;

		// Global AI Timers
		float actionCooldownTimer = 0.0f;
		float elapsedCombatTime = 0.0f;

		CombatBlackBoard(Actor* owner)
			: IComponent(owner)
		{
		}

		virtual ~CombatBlackBoard() = default;

		void Update(float /*deltaTime*/) override {}

		virtual bool HasAvailableStamina() const
		{
			return !useStamina || currentStamina > 0.0f;
		}

		virtual void ConsumeStamina(float cost, float regenDelay = -1.0f)
		{
			if (!useStamina || cost <= 0.0f) return;
			currentStamina = std::max(minStaminaFloor, currentStamina - cost);
			baseStaminaRegenTimer = (regenDelay >= 0.0f) ? regenDelay : baseStaminaRegenDelay;
		}

		nlohmann::json Serialize() override
		{
			nlohmann::json data = IComponent::Serialize();
			data["CurrentHealth"] = currentHealth;
			data["MaxHealth"] = maxHealth;
			data["CurrentSpeed"] = currentSpeed;
			data["CurrentTurnSpeed"] = currentTurnSpeed;
			data["AggroRange"] = aggroRange;
			data["SpawnPosition"] = nlohmann::json::array({ spawnPosition.x, spawnPosition.y, spawnPosition.z });
			data["HasSetSpawnPosition"] = hasSetSpawnPosition;
			data["UseStamina"] = useStamina;
			data["MaxStamina"] = maxStamina;
			data["CurrentStamina"] = currentStamina;
			return data;
		}

		void Deserialize(const nlohmann::json& data) override
		{
			IComponent::Deserialize(data);
			if (data.contains("CurrentHealth")) currentHealth = data["CurrentHealth"];
			if (data.contains("MaxHealth")) maxHealth = data["MaxHealth"];
			if (data.contains("CurrentSpeed")) currentSpeed = data["CurrentSpeed"];
			if (data.contains("CurrentTurnSpeed")) currentTurnSpeed = data["CurrentTurnSpeed"];
			if (data.contains("AggroRange")) aggroRange = data["AggroRange"];
			if (data.contains("HasSetSpawnPosition")) hasSetSpawnPosition = data["HasSetSpawnPosition"];
			if (data.contains("SpawnPosition") && data["SpawnPosition"].is_array() && data["SpawnPosition"].size() == 3)
			{
				spawnPosition = DirectX::SimpleMath::Vector3(data["SpawnPosition"][0], data["SpawnPosition"][1], data["SpawnPosition"][2]);
			}
			if (data.contains("UseStamina")) useStamina = data["UseStamina"];
			if (data.contains("MaxStamina")) maxStamina = data["MaxStamina"];
			if (data.contains("CurrentStamina")) currentStamina = data["CurrentStamina"];
		}

#if HEIN_WITH_EDITOR
		void OnInspectorGUI(GameContext& /*gameContext*/) override
		{
			if (ImGui::CollapsingHeader("CombatBlackBoard (Base)", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "AI Node: %s", activeNodeName.c_str());
				ImGui::DragFloat("Health", &currentHealth, 0.5f, 0.0f, 1000.0f);
				ImGui::Checkbox("Use Stamina", &useStamina);
				if (useStamina)
				{
					ImGui::DragFloat("Current Stamina", &currentStamina, 0.5f, minStaminaFloor, maxStamina);
					ImGui::DragFloat("Max Stamina", &maxStamina, 1.0f, 1.0f, 1000.0f);
					ImGui::DragFloat("Stamina Recovery (/s)", &baseStaminaRecoveryRate, 0.5f, 0.0f, 300.0f);
					ImGui::DragFloat("Stamina Regen Delay (s)", &baseStaminaRegenDelay, 0.05f, 0.0f, 5.0f);
				}
				ImGui::DragFloat("Speed", &currentSpeed, 0.5f, 0.0f, 200.0f);
				ImGui::DragFloat("Turn Speed", &currentTurnSpeed, 0.1f, 0.0f, 50.0f);
				ImGui::DragFloat("Aggro Range", &aggroRange, 1.0f, 10.0f, 300.0f);
				ImGui::DragFloat3("Spawn Position", &spawnPosition.x, 0.1f);
				ImGui::Checkbox("Is Grounded", &isGrounded);
				ImGui::Checkbox("Is Attacking", &isAttacking);
				ImGui::Checkbox("Is Leashing", &isLeashing);
				ImGui::Checkbox("Is Jumping", &isJumping);
				ImGui::Text("Target Distance: %.2f", distanceToTarget);
				ImGui::Text("Move Intent: (%.2f, %.2f, %.2f)", moveIntent.x, moveIntent.y, moveIntent.z);
				ImGui::Text("Look Intent: (%.2f, %.2f, %.2f)", lookIntent.x, lookIntent.y, lookIntent.z);
				ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", currentVelocity.x, currentVelocity.y, currentVelocity.z);
			}
		}
#else
		void OnInspectorGUI(GameContext& /*gameContext*/) override {}
#endif

		std::string GetComponentName() const override { return "CombatBlackBoard"; }
	};
}

