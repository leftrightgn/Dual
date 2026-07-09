#pragma once
#include <Components/IComponent.h>
#include <Entities/Actor.h>


namespace HEIN
{
	class ActorManager;
	struct CombatBlackBoard;
	class TransformComponent;
	class TargetTrackingComponent : public IComponent
	{
	private:

		HEIN::ActorManager* m_actorManager = nullptr;
		HEIN::CombatBlackBoard* m_blackboard = nullptr;
		HEIN::TransformComponent* m_transform = nullptr;

		HEIN::ActorType m_targetTypeToFind;

		HEIN::ActorID FindBestTarget() const;

	public:

		TargetTrackingComponent(Actor* owner, ActorManager* manager, HEIN::ActorType targetType);

		void Start() override;

		void Update(float deltaTime) override;


	};
}
