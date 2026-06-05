#include "pch.h"
#include "DebugUIManager.h"
#include "Components/SocketComponent.h"
#include "Components/ColliderComponent/OBBColliderComponent.h"
#include <ImGui/imgui.h>

namespace HEIN
{
	void DebugUIManager::Update(const GameContext& gameContext)
	{
		if (gameContext.keyboardTracker.pressed.F1)
		{
			m_isVisible = !m_isVisible;
		}
	}

	void DebugUIManager::Draw(Actor* player, Actor* sword)
	{
		if (!m_isVisible) return;

		ImGui::Begin("Engine Debug Tools");

		if (ImGui::CollapsingHeader("Player Sockets", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (player != nullptr)
			{
				HEIN::SocketComponent* socketComp = player->GetComponent<HEIN::SocketComponent>();
				if (socketComp != nullptr)
				{
					HEIN::Socket* weaponSocket = socketComp->GetSocket(L"WeaponSocket");
					if (weaponSocket != nullptr)
					{
						ImGui::Text("Weapon Socket Alignment");

						ImGui::DragFloat3("Local Position", &weaponSocket->localPosition.x, 0.01f);
						ImGui::DragFloat3("Local Rotation", &weaponSocket->localRotation.x, 0.01f);
					}
				}
			}
		}

		// ---------------------------------------------------------
		// 2. SWORD HITBOX TWEAKER
		// ---------------------------------------------------------
		if (ImGui::CollapsingHeader("Weapon Hitboxes", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (sword != nullptr)
			{
				HEIN::OBBColliderComponent* swordHitBox = sword->GetComponent<HEIN::OBBColliderComponent>();
				if (swordHitBox != nullptr)
				{
					ImGui::Text("Sword Hitbox Tuning");

					// --- 1. TWEAK OFFSET ---
					DirectX::SimpleMath::Vector3 currentOffset = swordHitBox->GetOffset();
					if (ImGui::DragFloat3("Offset (XYZ)", &currentOffset.x, 0.01f))
					{
						swordHitBox->SetOffset(currentOffset);
					}

					// --- 2. TWEAK ROTATION ---
					DirectX::SimpleMath::Vector3 currentRot = swordHitBox->GetRotationOffset();

					// Convert radians to degrees for the UI slider
					currentRot.x = DirectX::XMConvertToDegrees(currentRot.x);
					currentRot.y = DirectX::XMConvertToDegrees(currentRot.y);
					currentRot.z = DirectX::XMConvertToDegrees(currentRot.z);

					// DragFloat3 speed is 1.0f because degrees are whole numbers
					if (ImGui::DragFloat3("Rotation (Degrees)", &currentRot.x, 1.0f))
					{
						// Convert back to radians before giving it to the engine!
						currentRot.x = DirectX::XMConvertToRadians(currentRot.x);
						currentRot.y = DirectX::XMConvertToRadians(currentRot.y);
						currentRot.z = DirectX::XMConvertToRadians(currentRot.z);

						swordHitBox->SetRotationOffset(currentRot);
					}
				}
			}
		}

		ImGui::End();
	}
}