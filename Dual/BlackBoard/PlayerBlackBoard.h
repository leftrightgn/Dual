#pragma once
#include "CombatBlackBoard.h"
#include <algorithm>

namespace HEIN
{
	/**
	 * @brief PlayerBlackBoard extends CombatBlackBoard with Player-exclusive combat data
	 * (Shield Block Stamina, Guard Boost, Spontaneous Deflect, Partial Parry, Guard Counter).
	 *
	 * Keeps AI/Boss-specific fields (Behaviour Tree node names, tether leash, 6-permutation
	 * boss combo tables) out of the Player's memory footprint and Inspector UI.
	 */
	struct PlayerBlackBoard : public CombatBlackBoard
	{
		// 100-Point Guard Boost & Unified Stamina Model 
		float maxBlockStamina = 100.0f;
		float currentBlockStamina = 100.0f;
		float guardBoost = 55.0f;                 // percentage of stamina damage mitigated on standard block (0 to 100)
		float staminaRecoveryRate = 40.0f;        // Base stamina recovery per second when not blocking or attacking
		float blockRecoveryRate = 40.0f;          // Alias for backward compatibility (synced with staminaRecoveryRate)
		float blockHeldRecoveryRate = 6.0f;       // Slow stamina recovery per second while holding block
		float blockCooldownDuration = 0.18f;      // Short immunity per swing so multi-hit boss combos evaluate every hit
		float blockCooldownTimer = 0.0f;          // Debounce timer for incoming hits

		// Post-action delays before recovery begins
		float attackRegenDelay = 0.70f;           // Post-attack pause before stamina regen starts
		float dodgeRegenDelay = 0.45f;            // Post-dodge pause before stamina regen starts
		float exhaustedRegenDelay = 1.20f;        // Extended delay when completely exhausted (stamina <= 0)
		float staminaRegenDelayTimer = 0.0f;      // Current active countdown timer before recovery starts
		float guardBreakDelayTimer = 2.0f;        // Delay before stamina recovery begins after guard break
		bool isBlockBroken = false;

		// Deflect / Parry Windows (at 60fps)
		float parryStartupWindow = 0.0f;
		float parryActiveWindow = 0.20f;          // 12 frames at 60fps for Spontaneous Deflect / Perfect Parry
		float partialParryWindow = 0.32f;         // Early mistimed parry window
		float parryLockoutTimer = 0.0f;           // Set to 0.35f when exiting Block to prevent spam-tap parrying
		float blockActiveTimer = 0.0f;            // Time since entering block state

		// Guard Counter & Deflect Stacks
		float guardCounterWindowTimer = 0.0f;     // Set to 0.65f after successful block/deflect to allow Guard Counter input
		int deflectStacks = 0;                    // Consecutive deflects build bonus damage/knockback (+20% per stack)
		int maxDeflectStacks = 4;                 // Up to +80% bonus

		// Combat Costs & Absorption
		float dodgeStaminaCost = 15.0f;           // Flat stamina cost per dodge roll
		float lightAttackStaminaCost = 18.0f;     // Flat stamina cost per light attack (Slash1, Slash2)
		float comboFinisherStaminaCost = 25.0f;   // Flat stamina cost for 3rd hit finisher (Slash3)
		float guardPhysicalAbsorption = 0.85f;    // Percentage of physical damage absorbed on block (15% chip damage)

		// Data-Driven Defensive Tuning
		float parryStaminaRestore = 20.0f;
		float guardCounterWindowDuration = 0.65f;
		float partialParryStaminaMultiplier = 1.25f;
		float partialParryChipDamageRatio = 0.30f;
		float guardBreakChipDamageRatio = 0.25f;
		float guardBreakPushbackMultiplier = 1.20f;
		float partialParryPushbackMultiplier = 0.60f;
		float blockPushbackMultiplier = 0.35f;
		float heavyBlockPushbackMultiplier = 0.75f;
		float partialParryRegenDelay = 0.80f;
		float blockRegenDelay = 0.80f;
		int parryHitstopFrames = 8;
		int guardCounterHitstopFrames = 9;
		int comboFinisherHitstopFrames = 8;

		PlayerBlackBoard(Actor* owner)
			: CombatBlackBoard(owner)
		{
		}

		void OnInspectorGUI(GameContext& /*gameContext*/) override
		{
			if (ImGui::CollapsingHeader("PlayerBlackBoard", ImGuiTreeNodeFlags_DefaultOpen))
			{
				// -------------------------------------------------------------
				// Prominent & Manageable Stamina Recovery Controls
				// -------------------------------------------------------------
				ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.6f, 1.0f), "=== Stamina & Recovery Rates ===");

				// Live Visual Stamina Meter
				float staminaFraction = (maxBlockStamina > 0.0f) ? std::clamp(currentBlockStamina / maxBlockStamina, 0.0f, 1.0f) : 0.0f;
				char staminaOverlay[64];
				sprintf_s(staminaOverlay, "%.1f / %.1f (%.0f%%)", currentBlockStamina, maxBlockStamina, staminaFraction * 100.0f);
				ImVec4 barColor = (currentBlockStamina <= 0.0f || isBlockBroken) ? ImVec4(0.9f, 0.2f, 0.2f, 1.0f) : ImVec4(0.2f, 0.85f, 0.45f, 1.0f);
				ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
				ImGui::ProgressBar(staminaFraction, ImVec2(-1.0f, 20.0f), staminaOverlay);
				ImGui::PopStyleColor();

				// Base Stamina Recovery Rate
				if (ImGui::DragFloat("Stamina Recovery Rate (/s)", &staminaRecoveryRate, 0.5f, 0.0f, 300.0f, "%.1f pts/sec"))
				{
					blockRecoveryRate = staminaRecoveryRate;
				}
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Stamina recovered per second during normal movement (standing, walking, running).");
				}

				// Blocking Stamina Recovery Rate
				ImGui::DragFloat("Blocking Stamina Recovery (/s)", &blockHeldRecoveryRate, 0.5f, 0.0f, 150.0f, "%.1f pts/sec");
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Reduced stamina recovered per second while holding shield block.");
				}

				// Quick Tuning Presets
				ImGui::Text("Quick Recovery Presets:");
				ImGui::SameLine();
				if (ImGui::SmallButton("Fast (60/s)"))
				{
					staminaRecoveryRate = 60.0f;
					blockRecoveryRate = 60.0f;
					blockHeldRecoveryRate = 12.0f;
					attackRegenDelay = 0.40f;
					dodgeRegenDelay = 0.25f;
				}
				ImGui::SameLine();
				if (ImGui::SmallButton("Normal (40/s)"))
				{
					staminaRecoveryRate = 40.0f;
					blockRecoveryRate = 40.0f;
					blockHeldRecoveryRate = 6.0f;
					attackRegenDelay = 0.70f;
					dodgeRegenDelay = 0.45f;
				}
				ImGui::SameLine();
				if (ImGui::SmallButton("Hardcore (20/s)"))
				{
					staminaRecoveryRate = 20.0f;
					blockRecoveryRate = 20.0f;
					blockHeldRecoveryRate = 2.0f;
					attackRegenDelay = 1.00f;
					dodgeRegenDelay = 0.65f;
				}

				// Recovery Delays & Cooldowns
				if (ImGui::TreeNodeEx("Recovery Delays & Cooldowns", ImGuiTreeNodeFlags_None))
				{
					ImGui::DragFloat("Post-Attack Delay (s)", &attackRegenDelay, 0.05f, 0.0f, 5.0f, "%.2fs");
					ImGui::DragFloat("Post-Dodge Delay (s)", &dodgeRegenDelay, 0.05f, 0.0f, 5.0f, "%.2fs");
					ImGui::DragFloat("Exhaustion Penalty Delay (s)", &exhaustedRegenDelay, 0.05f, 0.0f, 5.0f, "%.2fs");
					ImGui::DragFloat("Guard Break Recovery Delay (s)", &guardBreakDelayTimer, 0.1f, 0.0f, 10.0f, "%.2fs");
					ImGui::DragFloat("Block Hit Cooldown (s)", &blockCooldownDuration, 0.01f, 0.01f, 1.0f, "%.2fs");
					ImGui::Text("Active Regen Pause Timer: %.2fs", staminaRegenDelayTimer);
					ImGui::TreePop();
				}

				ImGui::Separator();
				// -------------------------------------------------------------
				// Stamina Capacity & Action Costs
				// -------------------------------------------------------------
				ImGui::TextColored(ImVec4(0.3f, 0.85f, 1.0f, 1.0f), "Capacity & Action Costs");
				ImGui::DragFloat("Max Stamina", &maxBlockStamina, 1.0f, 10.0f, 1000.0f, "%.0f");
				ImGui::DragFloat("Current Stamina", &currentBlockStamina, 0.5f, -30.0f, maxBlockStamina, "%.1f");
				ImGui::DragFloat("Guard Boost (%)", &guardBoost, 0.5f, 0.0f, 100.0f, "%.1f%%");
				ImGui::DragFloat("Light Attack Stamina Cost", &lightAttackStaminaCost, 0.5f, 0.0f, 100.0f, "%.1f");
				ImGui::DragFloat("Combo Finisher Stamina Cost", &comboFinisherStaminaCost, 0.5f, 0.0f, 100.0f, "%.1f");
				ImGui::DragFloat("Dodge Stamina Cost", &dodgeStaminaCost, 0.5f, 0.0f, 100.0f, "%.1f");

				ImGui::Separator();
				// -------------------------------------------------------------
				// Deflect & Parry Timing
				// -------------------------------------------------------------
				ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Deflect & Parry Windows");
				ImGui::DragFloat("Parry Active Window (s)", &parryActiveWindow, 0.01f, 0.02f, 0.5f, "%.2fs");
				ImGui::DragFloat("Partial Parry Window (s)", &partialParryWindow, 0.01f, 0.05f, 0.8f, "%.2fs");
				ImGui::Text("Parry Lockout: %.2fs", parryLockoutTimer);
				ImGui::Text("Block Active Timer: %.2fs", blockActiveTimer);

				ImGui::Separator();
				// -------------------------------------------------------------
				// Guard Counter & Stacks
				// -------------------------------------------------------------
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.8f, 1.0f), "Guard Counter & Deflect Stacks");
				ImGui::Text("Guard Counter Window: %.2fs", guardCounterWindowTimer);
				ImGui::SliderInt("Deflect Stacks", &deflectStacks, 0, maxDeflectStacks);
				ImGui::Text("Guard Counter Bonus: +%d%%", deflectStacks * 20);

				ImGui::Separator();
				// -------------------------------------------------------------
				// General Locomotion & Status
				// -------------------------------------------------------------
				ImGui::DragFloat("Health", &currentHealth, 0.5f, 0.0f, 1000.0f);
				ImGui::DragFloat("Speed", &currentSpeed, 0.5f, 0.0f, 200.0f);
				ImGui::DragFloat("Turn Speed", &currentTurnSpeed, 0.1f, 0.0f, 50.0f);
				ImGui::Checkbox("Guard Broken", &isBlockBroken);
				ImGui::Checkbox("Is Grounded", &isGrounded);
				ImGui::Checkbox("Is Attacking", &isAttacking);
				ImGui::Checkbox("Is Jumping", &isJumping);
				ImGui::Text("Dodge Cooldown: %.2fs / %.2fs", dodgeCooldownTimer, maxDodgeCooldown);
				ImGui::Text("Move Intent: (%.2f, %.2f, %.2f)", moveIntent.x, moveIntent.y, moveIntent.z);
				ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", currentVelocity.x, currentVelocity.y, currentVelocity.z);
			}
		}

		nlohmann::json Serialize() override
		{
			nlohmann::json data = IComponent::Serialize();
			data["MaxStamina"] = maxBlockStamina;
			data["StaminaRecoveryRate"] = staminaRecoveryRate;
			data["BlockRecoveryRate"] = blockRecoveryRate;
			data["BlockHeldRecoveryRate"] = blockHeldRecoveryRate;
			data["GuardBoost"] = guardBoost;
			data["BlockCooldownDuration"] = blockCooldownDuration;
			data["AttackRegenDelay"] = attackRegenDelay;
			data["DodgeRegenDelay"] = dodgeRegenDelay;
			data["ExhaustedRegenDelay"] = exhaustedRegenDelay;
			data["GuardBreakDelayTimer"] = guardBreakDelayTimer;
			data["DodgeStaminaCost"] = dodgeStaminaCost;
			data["LightAttackStaminaCost"] = lightAttackStaminaCost;
			data["ComboFinisherStaminaCost"] = comboFinisherStaminaCost;
			data["GuardPhysicalAbsorption"] = guardPhysicalAbsorption;
			data["CurrentSpeed"] = currentSpeed;
			data["CurrentTurnSpeed"] = currentTurnSpeed;
			data["MaxHealth"] = maxHealth;
			data["ParryActiveWindow"] = parryActiveWindow;
			data["PartialParryWindow"] = partialParryWindow;
			return data;
		}

		void Deserialize(const nlohmann::json& data) override
		{
			IComponent::Deserialize(data);
			if (data.contains("MaxStamina")) maxBlockStamina = data["MaxStamina"];
			if (data.contains("StaminaRecoveryRate"))
			{
				staminaRecoveryRate = data["StaminaRecoveryRate"];
				blockRecoveryRate = staminaRecoveryRate;
			}
			else if (data.contains("BlockRecoveryRate"))
			{
				blockRecoveryRate = data["BlockRecoveryRate"];
				staminaRecoveryRate = blockRecoveryRate;
			}
			if (data.contains("BlockHeldRecoveryRate")) blockHeldRecoveryRate = data["BlockHeldRecoveryRate"];
			if (data.contains("GuardBoost")) guardBoost = data["GuardBoost"];
			if (data.contains("BlockCooldownDuration")) blockCooldownDuration = data["BlockCooldownDuration"];
			if (data.contains("AttackRegenDelay")) attackRegenDelay = data["AttackRegenDelay"];
			if (data.contains("DodgeRegenDelay")) dodgeRegenDelay = data["DodgeRegenDelay"];
			if (data.contains("ExhaustedRegenDelay")) exhaustedRegenDelay = data["ExhaustedRegenDelay"];
			if (data.contains("GuardBreakDelayTimer")) guardBreakDelayTimer = data["GuardBreakDelayTimer"];
			if (data.contains("DodgeStaminaCost")) dodgeStaminaCost = data["DodgeStaminaCost"];
			if (data.contains("LightAttackStaminaCost")) lightAttackStaminaCost = data["LightAttackStaminaCost"];
			if (data.contains("ComboFinisherStaminaCost")) comboFinisherStaminaCost = data["ComboFinisherStaminaCost"];
			if (data.contains("GuardPhysicalAbsorption")) guardPhysicalAbsorption = data["GuardPhysicalAbsorption"];
			if (data.contains("CurrentSpeed")) currentSpeed = data["CurrentSpeed"];
			if (data.contains("CurrentTurnSpeed")) currentTurnSpeed = data["CurrentTurnSpeed"];
			if (data.contains("MaxHealth")) maxHealth = data["MaxHealth"];
			if (data.contains("ParryActiveWindow")) parryActiveWindow = data["ParryActiveWindow"];
			if (data.contains("PartialParryWindow")) partialParryWindow = data["PartialParryWindow"];
		}

		std::string GetComponentName() const override { return "PlayerBlackBoard"; }
	};
}
