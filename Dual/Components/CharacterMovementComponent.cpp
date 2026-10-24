#include "pch.h"
#include "CharacterMovementComponent.h"
#include "BlackBoard/CombatBlackBoard.h"
#include "../../External/Engine/Components/RigidBodyComponent.h"
#include "../../External/Engine/Components/SkinnedModelComponent.h"
#include "../../External/Engine/Components/TransformComponent.h"
#include "../../External/Engine/Entities/Actor.h"
#include "../../External/Engine/ImGui/imgui.h"

HEIN::CharacterMovementComponent::CharacterMovementComponent(Actor* owner)
	: IComponent(owner)
	, m_blackboard()
	, m_transform()
	, m_friction(DEFAULT_FRICTION)
	, m_impulseVelocity(DirectX::SimpleMath::Vector3::Zero)
{
}

void HEIN::CharacterMovementComponent::Start()
{
	m_blackboard = m_owner ? m_owner->GetComponent<HEIN::CombatBlackBoard>() : nullptr;
	m_transform = m_owner ? m_owner->GetComponent<HEIN::TransformComponent>() : nullptr;
	m_rigidBody = m_owner ? m_owner->GetComponent<HEIN::RigidBodyComponent>() : nullptr;
	if (m_owner)
	{
		m_skinnedModels = m_owner->GetComponents<HEIN::SkinnedModelComponent>();
	}
}

void HEIN::CharacterMovementComponent::ApplyImpulse(const DirectX::SimpleMath::Vector3& impulse)
{
	m_impulseVelocity += impulse;
}

void HEIN::CharacterMovementComponent::Update(float deltaTime)
{
	if (!m_transform || !m_blackboard) return;

	// During Hitstop (Hit Freeze), preserve stored knockback m_impulseVelocity without friction decay
	// so the full kinetic knockback releases on the exact frame hitstop ends.
	if (m_owner != nullptr && m_owner->IsHitstopped())
	{
		return;
	}

	// Lazy refresh of cached components if invalidated or added post-Start
	if (!m_rigidBody && m_owner)
	{
		m_rigidBody = m_owner->GetComponent<HEIN::RigidBodyComponent>();
	}
	if (m_skinnedModels.empty() && m_owner)
	{
		m_skinnedModels = m_owner->GetComponents<HEIN::SkinnedModelComponent>();
	}

	// Pull root-motion state from the active visible SkinnedModelComponent into CombatBlackBoard
	m_blackboard->hasRootMotion = false;
	m_blackboard->rootMotionVelocity = DirectX::SimpleMath::Vector3::Zero;
	m_blackboard->rootMotionDelta = DirectX::SimpleMath::Vector3::Zero;

	for (auto* model : m_skinnedModels)
	{
		if (model != nullptr && model->IsVisible() && model->HasActiveRootMotion())
		{
			m_blackboard->hasRootMotion = true;
			m_blackboard->rootMotionVelocity = model->GetRootMotionVelocity();
			m_blackboard->rootMotionDelta = model->GetRootMotionDelta();
			break;
		}
	}

	// Decay external physical impulses (knockback, blast, etc.) smoothly via friction
	if (m_impulseVelocity.LengthSquared() > 0.0001f)
	{
		m_impulseVelocity = DirectX::SimpleMath::Vector3::Lerp(
			m_impulseVelocity,
			DirectX::SimpleMath::Vector3::Zero,
			deltaTime * m_friction
		);
		if (m_impulseVelocity.LengthSquared() <= 0.0001f)
		{
			m_impulseVelocity = DirectX::SimpleMath::Vector3::Zero;
		}
	}

	bool allowRootMotion = m_blackboard->hasRootMotion;

	if (!m_useLocomotionRootMotion)
	{
		// When locomotion root motion is disabled, only attack and jump animations drive root motion
		bool isAttackOrJump = (
			m_blackboard->isAttacking ||
			m_blackboard->isJumping ||
			m_blackboard->currentStance == HEIN::CombatStance::AttackRelese ||
			m_blackboard->currentStance == HEIN::CombatStance::OneHand ||
			m_blackboard->currentStance == HEIN::CombatStance::AttackWindup ||
			m_blackboard->currentStance == HEIN::CombatStance::AttackRecovery ||
			m_blackboard->currentStance == HEIN::CombatStance::Jumping
		);

		if (!isAttackOrJump)
		{
			allowRootMotion = false;
		}
	}

	float timeScale = (m_owner != nullptr) ? m_owner->GetEffectiveTimeScale() : 1.0f;

	// Check if root motion is actively driving movement (e.g. active root-motion model during attacks or jumps)
	if (allowRootMotion)
	{
		m_blackboard->currentVelocity = m_blackboard->rootMotionVelocity;
		DirectX::SimpleMath::Vector3 effectiveVelocity = m_blackboard->currentVelocity + m_impulseVelocity;

		if (m_rigidBody != nullptr && !m_rigidBody->isKinematic())
		{
			if (m_impulseVelocity.y > 0.01f)
			{
				m_rigidBody->SetVelocity(DirectX::SimpleMath::Vector3(effectiveVelocity.x, m_impulseVelocity.y, effectiveVelocity.z));
				m_impulseVelocity.y = 0.0f;
			}
			else
			{
				m_rigidBody->SetHorizontalVelocity(effectiveVelocity);
			}
		}
		else if (m_transform != nullptr)
		{
			m_transform->SetPosition(m_transform->GetPosition() + effectiveVelocity * deltaTime);
		}
	}
	else
	{
		// Stagger handling: cancel move intent if configured for this actor
		if (m_cancelMoveOnStagger && m_blackboard->currentStance == HEIN::CombatStance::Staggered)
		{
			m_blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;
		}

		// Scale desired velocity by local time-scale so anticipation wind-up pause (0.0f) also pauses forward lunge
		DirectX::SimpleMath::Vector3 desiredVelocity = m_blackboard->moveIntent * (m_blackboard->currentSpeed * timeScale);

		if (timeScale <= 0.001f)
		{
			m_blackboard->currentVelocity = DirectX::SimpleMath::Vector3::Zero;
		}
		else
		{
			m_blackboard->currentVelocity = DirectX::SimpleMath::Vector3::Lerp(
				m_blackboard->currentVelocity,
				desiredVelocity,
				deltaTime * m_friction
			);
		}

		DirectX::SimpleMath::Vector3 effectiveVelocity = m_blackboard->currentVelocity + m_impulseVelocity;

		// Move the actor
		if (m_rigidBody != nullptr)
		{
			if (m_impulseVelocity.y > 0.01f)
			{
				m_rigidBody->SetVelocity(DirectX::SimpleMath::Vector3(effectiveVelocity.x, m_impulseVelocity.y, effectiveVelocity.z));
				m_impulseVelocity.y = 0.0f;
			}
			else
			{
				m_rigidBody->SetHorizontalVelocity(effectiveVelocity);
			}
		}
		else if (m_transform != nullptr)
		{
			m_transform->SetPosition(m_transform->GetPosition() + effectiveVelocity * deltaTime);
		}
	}

	// Automatic animation blending for locomotion (e.g. Ghost or simple creatures)
	if (m_autoCrossfadeLocomotion)
	{
		auto* model = !m_skinnedModels.empty() ? m_skinnedModels[0] : nullptr;
		if (model)
		{
			if (m_blackboard->moveIntent.LengthSquared() > 0.01f)
			{
				if (model->HasAnimation(m_walkAnimName))
				{
					model->CrossfadeAnimation(m_walkAnimName, 0.15f);
				}
			}
			else
			{
				if (model->HasAnimation(m_idleAnimName))
				{
					model->CrossfadeAnimation(m_idleAnimName, 0.15f);
				}
			}
		}
	}
	const bool isLockedOn = m_blackboard->isLockedOn && m_allowLockOnFacing;
	if (isLockedOn && m_blackboard->isAttacking && m_blackboard->dirToTarget.LengthSquared() > 0.001f)
	{
		// Force character to face the lock-on target when locked on (idle, moving, or attacking)
		float targetYaw = atan2f(m_blackboard->dirToTarget.x, m_blackboard->dirToTarget.z);
		float modelOffset = DirectX::XM_PI;
		targetYaw += modelOffset;
		DirectX::SimpleMath::Vector3 currentRot = m_transform->GetRotationEuler();

		// Shortest path interpolation for yaw rotation
		float difference = targetYaw - currentRot.y;
		while (difference < -DirectX::XM_PI) difference += DirectX::XM_2PI;
		while (difference > DirectX::XM_PI) difference -= DirectX::XM_2PI;

		float turnSpeed = (m_blackboard->currentTurnSpeed > 0.0f) ? m_blackboard->currentTurnSpeed : 12.0f;
		turnSpeed = std::max(turnSpeed, 24.0f);
		currentRot.y += difference * turnSpeed * deltaTime;

		currentRot.x = 0.0f;
		currentRot.z = 0.0f;

		m_transform->SetRotationEuler(currentRot);
	}
	else if (m_blackboard->moveIntent.LengthSquared() > 0.001f)
	{
		// Rotate smoothly towards the current movement direction
		float targetYaw = atan2f(m_blackboard->moveIntent.x, m_blackboard->moveIntent.z);
		float modelOffset = DirectX::XM_PI;
		targetYaw += modelOffset;

		DirectX::SimpleMath::Vector3 currentRot = m_transform->GetRotationEuler();

		float difference = targetYaw - currentRot.y;
		while (difference < -DirectX::XM_PI) difference += DirectX::XM_2PI;
		while (difference > DirectX::XM_PI) difference -= DirectX::XM_2PI;

		float turnSpeed = (m_blackboard->currentTurnSpeed > 0.0f) ? m_blackboard->currentTurnSpeed : 10.0f;
		currentRot.y += difference * turnSpeed * deltaTime;

		currentRot.x = 0.0f;
		currentRot.z = 0.0f;

		m_transform->SetRotationEuler(currentRot);
	}
	else if (isLockedOn && m_blackboard->dirToTarget.LengthSquared() > 0.001f)
	{
		// Smoothly face the lock-on target when idle
		float targetYaw = atan2f(m_blackboard->dirToTarget.x, m_blackboard->dirToTarget.z);
		float modelOffset = DirectX::XM_PI;
		targetYaw += modelOffset;
		DirectX::SimpleMath::Vector3 currentRot = m_transform->GetRotationEuler();

		float difference = targetYaw - currentRot.y;
		while (difference < -DirectX::XM_PI) difference += DirectX::XM_2PI;
		while (difference > DirectX::XM_PI) difference -= DirectX::XM_2PI;

		float turnSpeed = (m_blackboard->currentTurnSpeed > 0.0f) ? m_blackboard->currentTurnSpeed : 8.0f;
		currentRot.y += difference * turnSpeed * deltaTime;

		currentRot.x = 0.0f;
		currentRot.z = 0.0f;

		m_transform->SetRotationEuler(currentRot);
	}
}

void HEIN::CharacterMovementComponent::OnInspectorGUI(GameContext& /*gameContext*/)
{
	if (ImGui::CollapsingHeader("CharacterMovementComponent", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat("Friction", &m_friction, 0.1f, 0.0f, 50.0f);
		char walkBuf[64];
		strcpy_s(walkBuf, m_walkAnimName.c_str());
		if (ImGui::InputText("Walk Anim", walkBuf, sizeof(walkBuf))) m_walkAnimName = walkBuf;
		char idleBuf[64];
		strcpy_s(idleBuf, m_idleAnimName.c_str());
		if (ImGui::InputText("Idle Anim", idleBuf, sizeof(idleBuf))) m_idleAnimName = idleBuf;
		ImGui::Text("Impulse: (%.2f, %.2f, %.2f)", m_impulseVelocity.x, m_impulseVelocity.y, m_impulseVelocity.z);
		if (m_blackboard)
		{
			ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", 
				m_blackboard->currentVelocity.x, m_blackboard->currentVelocity.y, m_blackboard->currentVelocity.z);
			ImGui::DragFloat("Move Speed", &m_blackboard->currentSpeed, 0.5f, 0.0f, 200.0f);
			ImGui::DragFloat("Turn Speed", &m_blackboard->currentTurnSpeed, 0.1f, 0.0f, 50.0f);
		}
		ImGui::Separator();
		ImGui::Text("Locomotion & Physics Behavior:");
		ImGui::Checkbox("Use Locomotion Root Motion", &m_useLocomotionRootMotion);
		ImGui::Checkbox("Cancel Move on Stagger", &m_cancelMoveOnStagger);
		ImGui::Checkbox("Auto Crossfade Locomotion", &m_autoCrossfadeLocomotion);
		ImGui::Checkbox("Allow Lock-On Facing", &m_allowLockOnFacing);
	}
}

nlohmann::json HEIN::CharacterMovementComponent::Serialize()
{
	nlohmann::json data = IComponent::Serialize();
	data["Friction"] = m_friction;
	data["WalkAnimName"] = m_walkAnimName;
	data["IdleAnimName"] = m_idleAnimName;
	data["UseLocomotionRootMotion"] = m_useLocomotionRootMotion;
	data["CancelMoveOnStagger"] = m_cancelMoveOnStagger;
	data["AutoCrossfadeLocomotion"] = m_autoCrossfadeLocomotion;
	data["AllowLockOnFacing"] = m_allowLockOnFacing;
	return data;
}

void HEIN::CharacterMovementComponent::Deserialize(const nlohmann::json& data)
{
	IComponent::Deserialize(data);
	if (data.contains("Friction")) m_friction = data["Friction"];
	if (data.contains("WalkAnimName")) m_walkAnimName = data["WalkAnimName"].get<std::string>();
	if (data.contains("IdleAnimName")) m_idleAnimName = data["IdleAnimName"].get<std::string>();
	if (data.contains("UseLocomotionRootMotion")) m_useLocomotionRootMotion = data["UseLocomotionRootMotion"];
	if (data.contains("CancelMoveOnStagger")) m_cancelMoveOnStagger = data["CancelMoveOnStagger"];
	if (data.contains("AutoCrossfadeLocomotion")) m_autoCrossfadeLocomotion = data["AutoCrossfadeLocomotion"];
	if (data.contains("AllowLockOnFacing")) m_allowLockOnFacing = data["AllowLockOnFacing"];
}
