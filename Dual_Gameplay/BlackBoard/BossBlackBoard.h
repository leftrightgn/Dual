#pragma once
#include "CombatBlackBoard.h"
#include <vector>

namespace HEIN
{
	/**
	 * @brief BossBlackBoard extends CombatBlackBoard with Boss/AI-exclusive data
	 * (Behaviour Tree debug node, spawn/tether leash, target stance perception,
	 * and the 6-permutation 3-hit combo pattern generator: 123, 132, 213, 231, 312, 321).
	 */
	struct BossBlackBoard : public CombatBlackBoard
	{
		// AI Behaviour Tree & Perception State
		std::string activeNodeName = "None";
		bool hasBeenSeenByPlayer = false;
		bool hasSetSpawnPosition = false;
		DirectX::SimpleMath::Vector3 spawnPosition = DirectX::SimpleMath::Vector3::Zero;
		float aggroRange = 80.0f;
		bool isLeashing = false;

		CombatStance targetStance = CombatStance::Idle;
		bool isTargetAttacking = false;
		float targetHealth = 100.0f;
		float actionCooldownTimer = 0.0f;

		// Multi-Parry Boss Threshold Tracking (Elden Ring Style)
		int currentParryCount = 0;
		int parriesRequiredForStagger = 2; // Phase 1
		int phase2ParriesRequired = 3;     // Phase 2
		float phase2HealthThresholdPct = 0.50f;
		float parryDecayTimer = 0.0f;
		float parryDecayDelay = 8.0f;      // Reset parries if player doesn't parry again within 8 seconds

		// Dynamic 3-Hit Combo Permutation System (123, 132, 213, 231, 312, 321)
		std::vector<std::string> activeComboSequence;
		std::string activeComboPatternName = "123";
		int comboIndex = 0;
		std::vector<int> comboPatternBag;
		int lastComboPatternIndex = -1;

		void RollNextBossComboPattern()
		{
			static const std::vector<std::vector<std::string>> kPatterns = {
				{ "Combo1", "Combo2", "Combo3" }, // 123
				{ "Combo1", "Combo3", "Combo2" }, // 132
				{ "Combo2", "Combo1", "Combo3" }, // 213
				{ "Combo2", "Combo3", "Combo1" }, // 231
				{ "Combo3", "Combo1", "Combo2" }, // 312
				{ "Combo3", "Combo2", "Combo1" }  // 321
			};
			static const char* kPatternNames[] = {
				"123", "132", "213", "231", "312", "321"
			};

			if (comboPatternBag.empty())
			{
				comboPatternBag = { 0, 1, 2, 3, 4, 5 };
				for (int i = 5; i > 0; --i)
				{
					int j = std::rand() % (i + 1);
					std::swap(comboPatternBag[i], comboPatternBag[j]);
				}
				if (comboPatternBag.back() == lastComboPatternIndex && comboPatternBag.size() > 1)
				{
					std::swap(comboPatternBag.back(), comboPatternBag[0]);
				}
			}

			int chosenIdx = comboPatternBag.back();
			comboPatternBag.pop_back();
			lastComboPatternIndex = chosenIdx;

			activeComboSequence = kPatterns[chosenIdx];
			activeComboPatternName = kPatternNames[chosenIdx];
			comboIndex = 0;
		}

		BossBlackBoard(Actor* owner)
			: CombatBlackBoard(owner)
		{
		}

		void OnInspectorGUI(GameContext& /*gameContext*/) override
		{
			if (ImGui::CollapsingHeader("BossBlackBoard", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "AI Node: %s", activeNodeName.c_str());
				if (!activeComboSequence.empty())
				{
					ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "Combo Pattern: %s (Step %d/%d)",
						activeComboPatternName.c_str(),
						comboIndex + 1,
						static_cast<int>(activeComboSequence.size()));
				}
				ImGui::Separator();
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f), "Multi-Parry Stagger Tracking");
				bool isPhase2 = (maxHealth > 0.0f) ? ((currentHealth / maxHealth) <= phase2HealthThresholdPct) : false;
				int reqParries = isPhase2 ? phase2ParriesRequired : parriesRequiredForStagger;
				ImGui::Text("Parry Count: %d / %d (%s)", currentParryCount, reqParries, isPhase2 ? "Phase 2" : "Phase 1");
				ImGui::DragInt("Parries Required (Phase 1)", &parriesRequiredForStagger, 1, 1, 10);
				float decayProgress = (parryDecayDelay > 0.0f) ? (parryDecayTimer / parryDecayDelay) : 0.0f;
				ImGui::ProgressBar(decayProgress, ImVec2(0.0f, 0.0f), "Parry Decay");
				ImGui::Text("Parry Decay Timer: %.2fs / %.2fs", parryDecayTimer, parryDecayDelay);
				ImGui::Separator();
				ImGui::DragFloat("Health", &currentHealth, 0.5f, 0.0f, 1000.0f);
				ImGui::DragFloat("Speed", &currentSpeed, 0.5f, 0.0f, 200.0f);
				ImGui::DragFloat("Turn Speed", &currentTurnSpeed, 0.1f, 0.0f, 50.0f);
				ImGui::DragFloat("Aggro Range", &aggroRange, 1.0f, 10.0f, 300.0f);
				ImGui::Checkbox("Is Grounded", &isGrounded);
				ImGui::Checkbox("Is Attacking", &isAttacking);
				ImGui::Checkbox("Is Leashing", &isLeashing);
				ImGui::Text("Distance To Target: %.2f", distanceToTarget);
				ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", currentVelocity.x, currentVelocity.y, currentVelocity.z);
			}
		}

		std::string GetComponentName() const override { return "BossBlackBoard"; }
	};
}
