#pragma once
#include <BehaviourTree/BTNode.h>

namespace HEIN
{
	class BTSequence : public BTNode
	{
	private:
		
		std::vector<std::unique_ptr<HEIN::BTNode>> m_children;
	public:

		void AddChild(std::unique_ptr<HEIN::BTNode> child);

		BTNodeState Tick(
			HEIN::Actor* self,
			HEIN::ActorManager* manager,
			HEIN::ActorID targetID,
			float deltaTime
		) override;
	};
}
