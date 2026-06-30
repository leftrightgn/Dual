#include "pch.h"
#include "DebugUIManager.h"
#include "Components/SocketComponent.h"
#include <Components/TransformComponent.h>
#include "Components/ColliderComponent/OBBColliderComponent.h"
#include "ImGui/imgui.h"
#include <Components/ColliderComponent/AABBColliderComponent.h>
#include <Entities/ActorManager.h>

namespace HEIN
{
	void DebugUIManager::Update(const GameContext& gameContext)
	{
		if (gameContext.keyboardTracker.pressed.F1)
		{
			m_isVisible = !m_isVisible;
		}
	}

	void DebugUIManager::Draw(HEIN::ActorManager& manager, Actor* player, Actor* sword, Actor* stage)
	{
		if (!m_isVisible) return;

		ImGui::Begin("Engine Debug Tools");

		// Player Socket
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

		// Sword HitBox
		if (ImGui::CollapsingHeader("Weapon Hitboxes", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (sword != nullptr)
			{
				HEIN::OBBColliderComponent* swordHitBox = sword->GetComponent<HEIN::OBBColliderComponent>();
				if (swordHitBox != nullptr)
				{
					ImGui::Text("Sword Hitbox Tuning");

					// TWEAK OFFSET ---
					DirectX::SimpleMath::Vector3 currentOffset = swordHitBox->GetOffset();
					if (ImGui::DragFloat3("Offset (XYZ)", &currentOffset.x, 0.01f))
					{
						swordHitBox->SetOffset(currentOffset);
					}

					// TWEAK ROTATION ---
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

		if (ImGui::CollapsingHeader("Foot Hitbox", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (player != nullptr)
			{
				std::vector<HEIN::OBBColliderComponent*> obbColliders = player->GetComponents<HEIN::OBBColliderComponent>();
				for (HEIN::OBBColliderComponent* currentBox : obbColliders)
				{
					if (currentBox->GetColliderTag() == L"RightFoot")
					{
						ImGui::Text("Right Foot Tuning");

						DirectX::SimpleMath::Vector3 currentOffset = currentBox->GetOffset();
						if (ImGui::DragFloat3("Right Offset XYZ", &currentOffset.x, 0.01f))
						{
							currentBox->SetOffset(currentOffset);
						}

						DirectX::SimpleMath::Vector3 currentExtents = currentBox->GetExtents();
						if (ImGui::DragFloat3("Right Extents", &currentExtents.x, 0.01f))
						{
							currentBox->SetExtents(currentExtents);
						}
					}
					else if (currentBox->GetColliderTag() == L"LeftFoot")
					{
						ImGui::Text("Left Foot Tuning");

						DirectX::SimpleMath::Vector3 currentOffset = currentBox->GetOffset();
						if (ImGui::DragFloat3("Left Offset XYZ", &currentOffset.x, 0.01f))
						{
							currentBox->SetOffset(currentOffset);
						}

						DirectX::SimpleMath::Vector3 currentExtents = currentBox->GetExtents();
						if (ImGui::DragFloat3("Left Extents", &currentExtents.x, 0.01f))
						{
							currentBox->SetExtents(currentExtents);
						}
					}
				} 
			}
		}
		if (ImGui::CollapsingHeader("Floor", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (stage != nullptr)
			{
				for (HEIN::ActorID childID : stage->GetChildren())
				{
					HEIN::Actor* childActor = manager.GetActor(childID);
					if (childActor == nullptr) continue;

					if (childActor->GetTag() == L"Floor")
					{
						HEIN::AABBColliderComponent* aabb = childActor->GetComponent<HEIN::AABBColliderComponent>();
						if (aabb != nullptr)
						{
							ImGui::Text("floor tunning");

							// TWEAK OFFSET
							DirectX::SimpleMath::Vector3 currentOffset = aabb->GetOffset();
							if (ImGui::DragFloat3("Offset(XYZ)##floor", &currentOffset.x, 0.01f))
							{
								aabb->SetOffset(currentOffset);
							}

							// TWEAK EXTENTS 
							DirectX::SimpleMath::Vector3 currentExtents = aabb->GetExtents();
							if (ImGui::DragFloat3("Extents (XYZ)", &currentExtents.x, 0.01f))
							{
								aabb->SetExtents(currentExtents);
							}
						}
						
					}

					else if (childActor->GetTag() == L"Pillar1")
					{
						ImGui::Text("Pillar1 Tunning");
						HEIN::TransformComponent* trans = childActor->GetComponent<HEIN::TransformComponent>();
						if (trans != nullptr)
						{
							DirectX::SimpleMath::Vector3 currentPosition = trans->GetPosition();
							if (ImGui::DragFloat3("PositionXYZ#Pillar", &currentPosition.x, 0.01f))
							{
								trans->SetPosition(currentPosition);
							}
						}
						
					}

				}

			}
		}

		ImGui::End();
	}
}