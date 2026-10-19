#pragma once
#include "../../External/Engine/Components/IComponent.h"
#include <SimpleMath.h>
#include <vector>

namespace HEIN
{
	struct CombatBlackBoard;
	class TransformComponent;
	class RigidBodyComponent;
	class SkinnedModelComponent;
	class Actor;

	/**
	 * @class CharacterMovementComponent
	 * @brief Handles logic related to moving a character actor within the game world.
	 * 
	 * Reads input or behavior decisions (via blackboard or inputs) and updates 
	 * the actor's TransformComponent appropriately while factoring in physics 
	 * like friction.
	 */
	// Handles character physics movement, rotation, and applying forces 
	// based on the actor's CombatBlackBoard state (moveIntent, currentStance, etc.).
	class CharacterMovementComponent : public HEIN::IComponent
	{
	private:
		/// @brief Default friction applied to stop the character when no input is given.
		static constexpr float DEFAULT_FRICTION = 20.0f;

	private:
		/// @brief Pointer to the combat blackboard, used to retrieve movement targets/states.
		HEIN::CombatBlackBoard* m_blackboard;

		/// @brief Pointer to the owner's transform, used to apply movement translation.
		HEIN::TransformComponent* m_transform;

		/// @brief Cached pointer to the owner's rigid body to eliminate per-frame GetComponent lookups.
		HEIN::RigidBodyComponent* m_rigidBody = nullptr;

		/// @brief Cached skinned model components on this actor to eliminate per-frame heap allocations.
		std::vector<HEIN::SkinnedModelComponent*> m_skinnedModels;

		/// @brief Current friction value applied to this character's movement.
		float m_friction;

		/// @brief External physical impulse velocity (hit knockback, blast, etc.) decaying via friction.
		DirectX::SimpleMath::Vector3 m_impulseVelocity;
		std::string m_walkAnimName = "Walk";
		std::string m_idleAnimName = "Idle";

	public:
		CharacterMovementComponent(Actor* owner);
	
		void Start() override;
	
		void Update(float deltaTime) override;

		/// @brief Applies an instantaneous physical impulse vector (e.g. hit knockback) to the character.
		void ApplyImpulse(const DirectX::SimpleMath::Vector3& impulse);

		DirectX::SimpleMath::Vector3 GetImpulseVelocity() const { return m_impulseVelocity; }
		void ClearImpulse() { m_impulseVelocity = DirectX::SimpleMath::Vector3::Zero; }

		const std::string& GetWalkAnimName() const { return m_walkAnimName; }
		void SetWalkAnimName(const std::string& name) { m_walkAnimName = name; }
		const std::string& GetIdleAnimName() const { return m_idleAnimName; }
		void SetIdleAnimName(const std::string& name) { m_idleAnimName = name; }

		void OnInspectorGUI(GameContext& gameContext) override;

		nlohmann::json Serialize() override;
		void Deserialize(const nlohmann::json& data) override;

		std::string GetComponentName() const override { return "CharacterMovementComponent"; }
	};
}