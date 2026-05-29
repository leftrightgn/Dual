#pragma once
#include "Camera/CameraController.h"
#include "Entities/Actor.h"
#include "GameContext.h"

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
	public:

		DebugDisplayController();
		~DebugDisplayController() = default;

		void Initialize();

		void Update(const GameContext& gameContext);

		void Render(GameContext& gameContext, const std::vector<std::unique_ptr<Actor>>& actors, Skybox* skybox);

		bool isMagnified() { return m_isMagnified; }

		bool isVisible() { return m_isVisible; }

		const DirectX::SimpleMath::Matrix GetViewMatrix() const;
		const DirectX::SimpleMath::Matrix GetProjMatrix() const;
	};
}

