#include "pch.h"
#include "ShockwaveEmitterComponent.h"
#include "../../../External/Engine/Entities/ActorManager.h"
#include "../../../External/Engine/Components/TransformComponent.h"
#include "../../../External/Engine/Components/HealthComponent.h"
#include "../../../External/Engine/Components/RigidBodyComponent.h"
#include "CombatStateMachineComponent.h"
#include "CharacterMovementComponent.h"
#include <BlackBoard/CombatBlackBoard.h>
#include <BlackBoard/PlayerBlackBoard.h>
#include <BlackBoard/BossBlackBoard.h>
#include "../../../External/Engine/Message/Messenger.h"
#include "../../../External/Engine/DebugingTools/DebugCollisionRenderer.h"
#include <algorithm>
#include "../../External/Engine/ImGui/imgui.h"

namespace HEIN
{
	ShockwaveEmitterComponent::ShockwaveEmitterComponent(Actor* owner, ActorManager* actorManager)
		: IComponent(owner)
		, m_actorManager(actorManager)
	{
		if (owner)
		{
			if (owner->GetActorType() == ActorType::Enemy)
			{
				m_defaultFilter = ShockwaveTargetFilter::OpponentsOnly;
			}
			else if (owner->GetActorType() == ActorType::Player)
			{
				m_defaultFilter = ShockwaveTargetFilter::OpponentsOnly;
			}
			else
			{
				m_defaultFilter = ShockwaveTargetFilter::AllCharacters;
			}
		}
	}

	void ShockwaveEmitterComponent::EmitShockwave(
		const DirectX::SimpleMath::Vector3& worldCenter,
		float maxRadius,
		float duration,
		float damage,
		float knockbackForce,
		bool isGroundOnly,
		ShockwaveTargetFilter filter,
		DirectX::XMVECTORF32 primaryColor,
		DirectX::XMVECTORF32 secondaryColor
	)
	{
		ShockwaveInstance wave;
		wave.center = worldCenter;
		wave.currentRadius = 1.0f;
		wave.maxRadius = maxRadius;
		wave.lifetime = 0.0f;
		wave.maxLifetime = (duration > 0.05f) ? duration : 1.1f;
		wave.damage = damage;
		wave.knockbackForce = knockbackForce;
		wave.waveThickness = 3.5f;
		wave.isGroundOnly = isGroundOnly;
		wave.targetFilter = filter;
		wave.primaryColor = primaryColor;
		wave.secondaryColor = secondaryColor;
		wave.isFinished = false;

		m_shockwaves.push_back(wave);
	}

	void ShockwaveEmitterComponent::EmitShockwaveFromOffset(
		const DirectX::SimpleMath::Vector3& localOffset,
		float maxRadius,
		float duration,
		float damage,
		float knockbackForce,
		bool isGroundOnly,
		ShockwaveTargetFilter filter,
		DirectX::XMVECTORF32 primaryColor,
		DirectX::XMVECTORF32 secondaryColor
	)
	{
		DirectX::SimpleMath::Vector3 spawnPos = localOffset;

		if (m_owner)
		{
			if (auto* trans = m_owner->GetComponent<TransformComponent>())
			{
				DirectX::SimpleMath::Vector3 forward = trans->GetForward();
				forward.y = 0.0f;
				if (forward.LengthSquared() > 0.001f) forward.Normalize();
				else forward = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 1.0f);

				DirectX::SimpleMath::Vector3 right = forward.Cross(DirectX::SimpleMath::Vector3(0.0f, 1.0f, 0.0f));

				DirectX::SimpleMath::Vector3 ownerPos = trans->GetPosition();
				spawnPos = ownerPos + right * localOffset.x + DirectX::SimpleMath::Vector3(0.0f, localOffset.y, 0.0f) + forward * localOffset.z;
				// Ground wave anchors to ground level beneath emitting character
				if (isGroundOnly)
				{
					spawnPos.y = ownerPos.y + localOffset.y;
				}
			}
		}

		EmitShockwave(spawnPos, maxRadius, duration, damage, knockbackForce, isGroundOnly, filter, primaryColor, secondaryColor);
	}

	void ShockwaveEmitterComponent::Clear()
	{
		m_shockwaves.clear();
	}

	void ShockwaveEmitterComponent::Update(float deltaTime)
	{
		for (auto& wave : m_shockwaves)
		{
			wave.lifetime += deltaTime;
			float t = std::clamp(wave.lifetime / wave.maxLifetime, 0.0f, 1.0f);
			wave.currentRadius = 1.0f + (wave.maxRadius - 1.0f) * t;

			if (wave.lifetime >= wave.maxLifetime)
			{
				wave.isFinished = true;
			}

			if (m_actorManager)
			{
				for (const auto& [id, actor] : m_actorManager->GetAllActors())
				{
					if (!actor || !actor->IsActive() || actor->GetID() == m_owner->GetID())
					{
						continue;
					}

					// Skip targets already processed for this specific shockwave
					if (wave.hitActors.find(actor->GetID()) != wave.hitActors.end())
					{
						continue;
					}

					if (!IsTargetValid(actor.get(), wave.targetFilter))
					{
						continue;
					}

					EvaluateShockwaveHit(wave, actor.get(), deltaTime);
				}
			}
		}

		m_shockwaves.erase(
			std::remove_if(m_shockwaves.begin(), m_shockwaves.end(),
				[](const ShockwaveInstance& w) { return w.isFinished; }),
			m_shockwaves.end()
		);
	}

	bool ShockwaveEmitterComponent::IsTargetValid(Actor* target, ShockwaveTargetFilter filter) const
	{
		if (!target || !m_owner) return false;

		ActorType myType = m_owner->GetActorType();
		ActorType targetType = target->GetActorType();

		switch (filter)
		{
		case ShockwaveTargetFilter::OpponentsOnly:
			if (myType == ActorType::Enemy)
			{
				return (targetType == ActorType::Player);
			}
			else if (myType == ActorType::Player)
			{
				return (targetType == ActorType::Enemy);
			}
			else
			{
				return (targetType == ActorType::Player || targetType == ActorType::Enemy);
			}

		case ShockwaveTargetFilter::PlayerOnly:
			return (targetType == ActorType::Player);

		case ShockwaveTargetFilter::EnemyOnly:
			return (targetType == ActorType::Enemy);

		case ShockwaveTargetFilter::AllCharacters:
			return (targetType == ActorType::Player || targetType == ActorType::Enemy);
		}

		return false;
	}

	void ShockwaveEmitterComponent::EvaluateShockwaveHit(ShockwaveInstance& wave, Actor* target, float /*deltaTime*/)
	{
		auto* targetTrans = target->GetComponent<TransformComponent>();
		if (!targetTrans) return;

		DirectX::SimpleMath::Vector3 targetPos = targetTrans->GetPosition();
		DirectX::SimpleMath::Vector3 diff = targetPos - wave.center;
		diff.y = 0.0f;
		float dist2D = diff.Length();

		// Elden Ring Hollow Wave-Front Band Check:
		// Active zone is a band between (R - thickness) and (R + buffer).
		// Once the expanding wave passes, the interior is safe.
		float minBand = std::max(0.0f, wave.currentRadius - wave.waveThickness);
		float maxBand = wave.currentRadius + 1.5f;

		if (dist2D < minBand || dist2D > maxBand)
		{
			return; // Target is either inside the cleared core or outside the wavefront
		}

		if (wave.isGroundOnly)
		{
			// Check Jump Evade (Elden Ring Jump Dodge)
			bool isJumping = false;
			auto* targetBB = target->GetComponent<CombatBlackBoard>();
			auto* targetPlBB = target->GetComponent<PlayerBlackBoard>();
			auto* targetSM = target->GetComponent<CombatStateMachineComponent>();

			if (targetBB && (targetBB->isJumping || targetBB->currentStance == CombatStance::Jumping))
			{
				isJumping = true;
			}
			if (targetSM && targetSM->GetCurrentStateName() == "Jump")
			{
				isJumping = true;
			}
			if (targetPos.y > (wave.center.y + 1.2f))
			{
				isJumping = true;
			}
			if (auto* rb = target->GetComponent<RigidBodyComponent>())
			{
				if (rb->GetVelocity().y > 0.5f)
				{
					isJumping = true;
				}
			}

			if (isJumping)
			{
				// Clean Elden Ring Jump Evade:
				// Mark as evaluated so when player lands in the hollow core later, they remain unharmed!
				wave.hitActors.insert(target->GetID());
				return;
			}

			// Grounded Impact: Player failed to jump over the ground shockwave
			wave.hitActors.insert(target->GetID());

			// Option A: Ground shockwaves cannot be blocked with a shield -> Instant Guard Break
			if (targetSM && targetSM->IsBlocking())
			{
				if (targetPlBB)
				{
					targetPlBB->currentBlockStamina = 0.0f;
					targetPlBB->isBlockBroken = true;
					targetPlBB->deflectStacks = 0;
					targetPlBB->guardCounterWindowTimer = 0.0f;
					targetPlBB->staminaRegenDelayTimer = targetPlBB->guardBreakDelayTimer;
				}
				targetSM->ChangeState("Stun", 0.05f);
				Messenger::GetInstance()->Notify(target->GetID(), Message::HIT_STUN);
			}

			// Apply Damage
			if (auto* health = target->GetComponent<HealthComponent>())
			{
				health->ApplyDamage(wave.damage);
			}

			// Elden Ring Upward Launch + Radial Blast Impulse
			DirectX::SimpleMath::Vector3 pushDir = diff;
			if (pushDir.LengthSquared() > 0.001f)
			{
				pushDir.Normalize();
			}
			else
			{
				pushDir = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 1.0f);
			}

			DirectX::SimpleMath::Vector3 blastImpulse = pushDir * wave.knockbackForce;
			blastImpulse.y = wave.knockbackForce; // Upward launch

			if (auto* moveComp = target->GetComponent<CharacterMovementComponent>())
			{
				moveComp->ApplyImpulse(blastImpulse);
			}
			else if (auto* rb = target->GetComponent<RigidBodyComponent>())
			{
				rb->SetHorizontalVelocity(rb->GetVelocity() + blastImpulse);
			}

			// Hit Confirmation for Attacker
			if (m_owner)
			{
				if (auto* attBB = m_owner->GetComponent<CombatBlackBoard>())
				{
					attBB->lastAttackHit = true;
				}
				Messenger::GetInstance()->Notify(m_owner->GetID(), Message::ATTACK_HIT_CONFIRMED);
			}
		}
		else
		{
			// 3D Spherical Blast (e.g. Explosive Barrel / Bomb)
			float dist3D = (targetPos - wave.center).Length();
			if (dist3D <= wave.currentRadius)
			{
				wave.hitActors.insert(target->GetID());

				if (auto* health = target->GetComponent<HealthComponent>())
				{
					health->ApplyDamage(wave.damage);
				}

				DirectX::SimpleMath::Vector3 pushDir = targetPos - wave.center;
				if (pushDir.LengthSquared() > 0.001f) pushDir.Normalize();
				else pushDir = DirectX::SimpleMath::Vector3(0.0f, 1.0f, 0.0f);

				DirectX::SimpleMath::Vector3 blastImpulse = pushDir * wave.knockbackForce;

				if (auto* moveComp = target->GetComponent<CharacterMovementComponent>())
				{
					moveComp->ApplyImpulse(blastImpulse);
				}
				else if (auto* rb = target->GetComponent<RigidBodyComponent>())
				{
					rb->SetHorizontalVelocity(rb->GetVelocity() + blastImpulse);
				}

				if (m_owner)
				{
					if (auto* attBB = m_owner->GetComponent<CombatBlackBoard>())
					{
						attBB->lastAttackHit = true;
					}
					Messenger::GetInstance()->Notify(m_owner->GetID(), Message::ATTACK_HIT_CONFIRMED);
				}
			}
		}
	}

	void ShockwaveEmitterComponent::Draw(
		GameContext& gameContext,
		const DirectX::SimpleMath::Matrix& /*world*/,
		const DirectX::SimpleMath::Matrix& /*view*/,
		const DirectX::SimpleMath::Matrix& /*proj*/
	)
	{
		// Only render collider wireframe if collider debugging is active (F1) or explicitly toggled on
		if (!gameContext.showColliders && !m_drawDebugWireframe) return;
		if (gameContext.debugCollisionRenderer == nullptr) return;

		for (const auto& wave : m_shockwaves)
		{
			DrawShockwaveWireframe(gameContext, wave);
		}
	}

	void ShockwaveEmitterComponent::DrawShockwaveWireframe(GameContext& gameContext, const ShockwaveInstance& wave)
	{
		float R = wave.currentRadius;
		if (R <= 0.01f) return;

		const int segments = 36;

		// Primary Ground Wireframe Ring (36 segments) at ground elevation
		for (int i = 0; i < segments; ++i)
		{
			float angle1 = (DirectX::XM_2PI / segments) * static_cast<float>(i);
			float angle2 = (DirectX::XM_2PI / segments) * static_cast<float>(i + 1);
			DirectX::SimpleMath::Vector3 p1 = wave.center + DirectX::SimpleMath::Vector3(cosf(angle1) * R, 0.25f, sinf(angle1) * R);
			DirectX::SimpleMath::Vector3 p2 = wave.center + DirectX::SimpleMath::Vector3(cosf(angle2) * R, 0.25f, sinf(angle2) * R);
			gameContext.debugCollisionRenderer->QueueLine(p1, p2, wave.primaryColor);
		}

		// Multi-Latitude Horizontal Dome Rings (at 25%, 50%, and 75% dome height)
		float heights[3] = { 0.25f, 0.50f, 0.75f };
		for (int h = 0; h < 3; ++h)
		{
			float yOffset = R * heights[h];
			float ringRadius = R * sqrtf(std::max(0.0f, 1.0f - heights[h] * heights[h]));
			for (int i = 0; i < segments; ++i)
			{
				float angle1 = (DirectX::XM_2PI / segments) * static_cast<float>(i);
				float angle2 = (DirectX::XM_2PI / segments) * static_cast<float>(i + 1);
				DirectX::SimpleMath::Vector3 p1 = wave.center + DirectX::SimpleMath::Vector3(cosf(angle1) * ringRadius, yOffset, sinf(angle1) * ringRadius);
				DirectX::SimpleMath::Vector3 p2 = wave.center + DirectX::SimpleMath::Vector3(cosf(angle2) * ringRadius, yOffset, sinf(angle2) * ringRadius);
				gameContext.debugCollisionRenderer->QueueLine(p1, p2, wave.secondaryColor);
			}
		}

		// Vertical Longitudinal Arches / Ribs (8 Arches arching over the dome to apex)
		const int archCount = 8;
		const int archSteps = 16;
		for (int a = 0; a < archCount; ++a)
		{
			float archAngle = (DirectX::XM_PI / archCount) * static_cast<float>(a);
			DirectX::SimpleMath::Vector3 dir(cosf(archAngle), 0.0f, sinf(archAngle));

			for (int s = 0; s < archSteps; ++s)
			{
				float phi1 = (DirectX::XM_PI / archSteps) * static_cast<float>(s);
				float phi2 = (DirectX::XM_PI / archSteps) * static_cast<float>(s + 1);

				DirectX::SimpleMath::Vector3 p1 = wave.center + dir * (cosf(phi1) * R) + DirectX::SimpleMath::Vector3(0.0f, sinf(phi1) * R, 0.0f);
				DirectX::SimpleMath::Vector3 p2 = wave.center + dir * (cosf(phi2) * R) + DirectX::SimpleMath::Vector3(0.0f, sinf(phi2) * R, 0.0f);
				gameContext.debugCollisionRenderer->QueueLine(p1, p2, wave.secondaryColor);
			}
		}

		// Expanding Inner Concentric Ripple Ring
		if (R > 4.0f)
		{
			float innerR = R * 0.65f;
			for (int i = 0; i < segments; ++i)
			{
				float angle1 = (DirectX::XM_2PI / segments) * static_cast<float>(i);
				float angle2 = (DirectX::XM_2PI / segments) * static_cast<float>(i + 1);
				DirectX::SimpleMath::Vector3 p1 = wave.center + DirectX::SimpleMath::Vector3(cosf(angle1) * innerR, 0.25f, sinf(angle1) * innerR);
				DirectX::SimpleMath::Vector3 p2 = wave.center + DirectX::SimpleMath::Vector3(cosf(angle2) * innerR, 0.25f, sinf(angle2) * innerR);
				gameContext.debugCollisionRenderer->QueueLine(p1, p2, DirectX::Colors::Yellow);
			}
		}
	}

	void ShockwaveEmitterComponent::OnInspectorGUI(GameContext& /*gameContext*/)
	{
		ImGui::Text("Shockwave Emitter Component");
		ImGui::Text("Active Waves: %zu", m_shockwaves.size());
		ImGui::Checkbox("Show Shockwave Collider Wireframe", &m_drawDebugWireframe);

		if (ImGui::Button("Emit Test Shockwave (Forward 10m)"))
		{
			EmitShockwaveFromOffset(DirectX::SimpleMath::Vector3(0.0f, 0.0f, 10.0f), 24.0f, 1.1f, 45.0f, 35.0f, true);
		}

		if (ImGui::Button("Clear Shockwaves"))
		{
			Clear();
		}
	}
}
