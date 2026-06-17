#pragma once
#include "Camera/CameraController.h"
#include "Entities/Actor.h"
#include "Framework/GameContext.h"
#include "DebugUIManager.h"

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
		Actor* m_debugPlayer = nullptr;
		Actor* m_debugSword = nullptr;
		Actor* m_debugStage = nullptr;
	public:

		DebugDisplayController();
		~DebugDisplayController() = default;

		void Initialize();

		void Update(const GameContext& gameContext);

		void Render(
			GameContext& gameContext,
			const std::vector<std::unique_ptr<Actor>>& actors, 
			Skybox* skybox,
			DirectX::SimpleMath::Matrix mainView,
			DirectX::SimpleMath::Matrix mainProj);

		bool isMagnified() { return m_isMagnified; }

		bool isVisible() { return m_isVisible; }

		const DirectX::SimpleMath::Matrix GetViewMatrix() const;
		const DirectX::SimpleMath::Matrix GetProjMatrix() const;

		void SetDebugTargets(Actor* player, Actor* sword, Actor* stage);

	};
}

