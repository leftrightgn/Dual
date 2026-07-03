#pragma once
#include <Components/IComponent.h>
#include <Entities/Actor.h>


namespace HEIN
{
	class ActorManager;
	class CombatBlackBoard;
	class TransformComponent;
	class TargetTrackingComponent : public IComponent
	{
	private:

		HEIN::ActorManager* m_actorManager = nullptr;
		HEIN::CombatBlackBoard* m_blackBoard = nullptr;
		HEIN::TransformComponent* m_transform = nullptr;

		HEIN::ActorID FindBestTarget() const;

	public:

		TargetTrackingComponent(Actor* owner, ActorManager* manager);

		void Start() override;

		void Update(float deltaTime) override;


	};
}
