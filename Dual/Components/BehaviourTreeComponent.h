#pragma once
#include <Components/IComponent.h>
#include <BehaviourTree/BTNode.h>

namespace HEIN
{
	class BehaviourTreeComponent : public IComponent
	{
	private:

		std::unique_ptr<BTNode> m_rootNode;
		HEIN::ActorManager* m_actorManager;
		HEIN::ActorID m_targetID;

	public:

		BehaviourTreeComponent(Actor* owner);

		void Initialize(
			std::unique_ptr<BTNode> root,
			HEIN::ActorManager* manager,
			HEIN::ActorID targetID
		);

		void Start() override {}
		void Update(float deltaTime) override;

	};
}