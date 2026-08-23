#pragma once
#include "../../External/Engine/Components/IComponent.h"

namespace HEIN
{
	struct CombatBlackBoard;
	class TransformComponent;
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
		static constexpr float DEFAULT_FRICTION = 8.0f;

	private:
		/// @brief Pointer to the combat blackboard, used to retrieve movement targets/states.
		HEIN::CombatBlackBoard* m_blackboard;

		/// @brief Pointer to the owner's transform, used to apply movement translation.
		HEIN::TransformComponent* m_transform;

		/// @brief Current friction value applied to this character's movement.
		float m_friction;

	public:
		CharacterMovementComponent(Actor* owner);
	
		void Start() override;
	
		void Update(float deltaTime) override;
		
	};
}