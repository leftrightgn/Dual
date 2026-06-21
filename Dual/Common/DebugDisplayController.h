#pragma once
#include "Camera/CameraController.h"
#include "Entities/Actor.h"
#include "Framework/GameContext.h"
#include "DebugUIManager.h"
#include <Entities/ActorManager.h>

namespace HEIN
{
	class Skybox;

	class DebugDisplayController
	{
	private:

		std::unique_ptr<CameraController> m_debugcameraController;

		bool m_isVisible;
		bool m_isMagnified;

		DirectX::SimpleMath::Matrix m_projMatrix;

		float m_virtualMouseX = 0.0f;
		float m_virtualMouseY = 0.0f;

		DebugUIManager m_debugUI;
		HEIN::ActorID m_debugPlayerID = HEIN::INVALID_ACTOR_ID;
		HEIN::ActorID m_debugSwordID = HEIN::INVALID_ACTOR_ID;
		HEIN::ActorID m_debugStageID = HEIN::INVALID_ACTOR_ID;
	public:

		DebugDisplayController();
		~DebugDisplayController() = default;

		void Initialize();

		void Update(const GameContext& gameContext);

		void Render(
			GameContext& gameContext,
			HEIN::ActorManager& actorManager,
			Skybox* skybox,
			DirectX::SimpleMath::Matrix mainView,
			DirectX::SimpleMath::Matrix mainProj);

		bool isMagnified() { return m_isMagnified; }

		bool isVisible() { return m_isVisible; }

		const DirectX::SimpleMath::Matrix GetViewMatrix() const;
		const DirectX::SimpleMath::Matrix GetProjMatrix() const;

		void SetDebugTargets(HEIN::ActorID playerID, HEIN::ActorID swordID, HEIN::ActorID stageID);

	};
}

