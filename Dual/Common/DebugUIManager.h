#pragma once
#include "Framework/GameContext.h"
#include "Entities/Actor.h"
#include <vector>
#include <memory>

namespace HEIN
{
	class ActorManager;

	class DebugUIManager
	{
	private:
		bool m_isVisible = true;

	public:
		DebugUIManager() = default;
		~DebugUIManager() = default;

		void Update(const GameContext& gameContext);

		void Draw(HEIN::ActorManager& manager, Actor* player, Actor* sword, Actor* stageRoot);
	};
}