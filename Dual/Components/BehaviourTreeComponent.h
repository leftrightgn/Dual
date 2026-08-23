#pragma once
#include "../../External/Engine/Components/IComponent.h"
#include "../../External/Engine/BehaviourTree/BTNode.h"
#include "../../External/Engine/Entities/Actor.h"
#include "../../External/Engine/Entities/ActorManager.h"

namespace HEIN
{
	/**
	 * @class BehaviourTreeComponent
	 * @brief Acts as the "Brain" of the AI, executing a hierarchical set of conditions and tasks (Behaviour Tree).
	 * 
	 * ARCHITECTURE & DATA FLOW:
	 * Evaluates the AI logic by traversing the tree from the root node downward every tick (e.g., TargetTracking -> BlackBoard -> BTSelector -> Sequences -> Action Nodes).
	 * 
	 * DECOUPLING & BLACKBOARD USAGE:
	 * The Behaviour Tree does not manage physics or animation states directly. Instead, Action Nodes within the tree 
	 * communicate via the `CombatBlackBoard`. The BT reads environmental data (like distance to target calculated by TargetTrackingComponent)
	 * and writes intent vectors and states. The FSM (CombatStateMachineComponent) then consumes the Blackboard state 
	 * to drive actual movement and attacks.
	 */
	class BehaviourTreeComponent : public IComponent
	{
	private:
		/// @brief The root node of this actor's behaviour tree.
		std::unique_ptr<BTNode> m_rootNode;

		/// @brief Pointer to the actor manager for looking up other entities in the world.
		HEIN::ActorManager* m_actorManager;

		/// @brief The ID of the target actor (e.g. the player) this AI is focusing on.
		HEIN::ActorID m_targetID;

	public:
		BehaviourTreeComponent(Actor* owner);

		/**
		 * @brief Initializes the component with a behaviour tree and its target.
		 * @param root The root node of the behaviour tree to execute.
		 * @param manager The actor manager used to find other entities.
		 * @param targetID The specific ActorID this AI will track or attack.
		 */
		void Initialize(
			std::unique_ptr<BTNode> root,
			HEIN::ActorManager* manager,
			HEIN::ActorID targetID
		);

		void Start() override {}
		
		/**
		 * @brief Evaluates the behaviour tree every frame.
		 * @param deltaTime Time elapsed since the last frame.
		 */
		void Update(float deltaTime) override;

	};
}