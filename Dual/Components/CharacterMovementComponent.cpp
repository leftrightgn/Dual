#include "pch.h"
#include "CharacterMovementComponent.h"
#include "../../External/Engine/Components/TransformComponent.h"
#include "BlackBoard/CombatBlackBoard.h"
#include "../../External/Engine/Entities/Actor.h"
#include "../../External/Engine/Components/RigidBodyComponent.h"
#include "../../External/Engine/Components/SkinnedModelComponent.h"
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

	bool isEnemy = (m_owner != nullptr && m_owner->GetActorType() == HEIN::ActorType::Enemy);
	bool allowRootMotion = m_blackboard->hasRootMotion;

	if (isEnemy)
	{
		// For the Enemy: do not use root motion for regular movement (Walk, Strafe, etc.).
		// But when jumping or attacking: if there is root motion in the animation, use it!
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
		// FAST-ACTION COMBAT: Only cancel movement if player is staggered! Attacks can move fluidly!
		if (!isEnemy && m_blackboard->currentStance == HEIN::CombatStance::Staggered)
		{
			m_blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;
		}

		// Dynamic Speed: Calculate desired velocity and apply friction interpolation
		DirectX::SimpleMath::Vector3 desiredVelocity = m_blackboard->moveIntent * m_blackboard->currentSpeed;

		m_blackboard->currentVelocity = DirectX::SimpleMath::Vector3::Lerp(
			m_blackboard->currentVelocity,
			desiredVelocity,
			deltaTime * m_friction
		);

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

	// Automatic animation blending for Ghost form
	if (m_owner != nullptr && m_owner->GetActorType() == HEIN::ActorType::Ghost)
	{
		auto* model = !m_skinnedModels.empty() ? m_skinnedModels[0] : nullptr;
		if (model)
		{
			if (m_blackboard->moveIntent.LengthSquared() > 0.01f)
			{
				if (model->HasAnimation("Walk"))
				{
					model->CrossfadeAnimation("Walk", 0.15f);
				}
			}
			else
			{
				if (model->HasAnimation("Idle"))
				{
					model->CrossfadeAnimation("Idle", 0.15f);
				}
			}
		}
	}
	if (m_blackboard->isLockedOn && m_blackboard->currentStance == HEIN::CombatStance::Strafing &&  m_blackboard->dirToTarget.LengthSquared() > 0.001f)
	{
		// Force character to face the lock-on target while strafing
		float targetYaw = atan2f(m_blackboard->dirToTarget.x, m_blackboard->dirToTarget.z);
		float modelOffset = DirectX::XM_PI;
		targetYaw += modelOffset;
		DirectX::SimpleMath::Vector3 currentRot = m_transform->GetRotationEuler();

		// Shortest path interpolation for yaw rotation
		float difference = targetYaw - currentRot.y;
		while (difference < -DirectX::XM_PI) difference += DirectX::XM_2PI;
		while (difference > DirectX::XM_PI) difference -= DirectX::XM_2PI;

		currentRot.y += difference * m_blackboard->currentTurnSpeed * deltaTime;

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
}

void HEIN::CharacterMovementComponent::OnInspectorGUI(GameContext& /*gameContext*/)
{
	if (ImGui::CollapsingHeader("CharacterMovementComponent", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat("Friction", &m_friction, 0.1f, 0.0f, 50.0f);
		ImGui::Text("Impulse: (%.2f, %.2f, %.2f)", m_impulseVelocity.x, m_impulseVelocity.y, m_impulseVelocity.z);
		if (m_blackboard)
		{
			ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", 
				m_blackboard->currentVelocity.x, m_blackboard->currentVelocity.y, m_blackboard->currentVelocity.z);
			ImGui::DragFloat("Move Speed", &m_blackboard->currentSpeed, 0.5f, 0.0f, 200.0f);
			ImGui::DragFloat("Turn Speed", &m_blackboard->currentTurnSpeed, 0.1f, 0.0f, 50.0f);
		}
	}
}

nlohmann::json HEIN::CharacterMovementComponent::Serialize()
{
	nlohmann::json data = IComponent::Serialize();
	data["Friction"] = m_friction;
	return data;
}

void HEIN::CharacterMovementComponent::Deserialize(const nlohmann::json& data)
{
	IComponent::Deserialize(data);
	if (data.contains("Friction")) m_friction = data["Friction"];
}
