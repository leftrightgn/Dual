#pragma once
#include <vector>
#include <memory>
#include "Framework/GameContext.h"
#include "CollisionDispatcher.h"

namespace HEIN
{
	class Actor;
	class ColliderComponent;
    
    class PhysicsSystem
	{
	public:
		void UpdateMovement(GameContext& gameContext, std::vector<std::unique_ptr<HEIN::Actor>>& actors, float deltaTime);
		void UpdateCollisions(GameContext& gameContext, std::vector<std::unique_ptr<HEIN::Actor>>& actors, float deltaTime);
	private:
		void ResolvePhysicalOverlap(HEIN::ColliderComponent* colA, HEIN::ColliderComponent* colB, const HEIN::CollisionManifold& mainfold);
	};
}

