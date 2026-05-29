#include "pch.h"
#include "DebugDisplayController.h"
#include <Camera/DebugCameraMode.h>
#include <Camera/CameraController.h>
#include "Effect/Skybox.h"
#include <Entities/Actor.h>

namespace HEIN
{
	DebugDisplayController::DebugDisplayController()
		: m_isVisible(true)
		, m_isMagnified(false)
	{
	}

	void DebugDisplayController::Initialize()
	{
		m_debugcameraController = std::make_unique<CameraController>();

		m_debugcameraController->RegisterCamera(HEIN::CameraType::Debug, []() { return std::make_unique<HEIN::DebugCameraMode>(); });

		m_debugcameraController->SetFirstCamera(CameraType::Debug);
	}

	void DebugDisplayController::Update(const GameContext& gameContext)
	{
		if (gameContext.keyboardTracker.pressed.F2) m_isMagnified = !m_isMagnified;
		if (gameContext.keyboardTracker.pressed.F3) m_isVisible = !m_isVisible;

		const float deltaTime = static_cast<float>(gameContext.timer.GetFramesPerSecond());

		m_debugcameraController->Update(deltaTime);

		if (m_isMagnified)
		{
			CameraInputState debugInput;
			DirectX::Mouse::State mouseState = DirectX::Mouse::Get().GetState();
			debugInput.mouseX = static_cast<float>(mouseState.x);
			debugInput.mouseY = static_cast<float>(mouseState.y);
			debugInput.isLeftMouseDown = mouseState.leftButton;
			debugInput.scrollWheelDelta = mouseState.scrollWheelValue;
			m_debugcameraController->ProcessInput(debugInput);
		}
	}

	void DebugDisplayController::Render(GameContext& gameContext, const std::vector<std::unique_ptr<Actor>>& actors, Skybox* skybox)
	{
		if (!m_isVisible) return;

		ID3D11DeviceContext* context = gameContext.deviceResources.GetD3DDeviceContext();
		ID3D11DepthStencilView* dsv = gameContext.deviceResources.GetDepthStencilView();
		D3D11_VIEWPORT fullscreen = gameContext.deviceResources.GetScreenViewport();
		D3D11_VIEWPORT debugViewport;

		if (m_isMagnified)
		{
			debugViewport = fullscreen;
		}
		else
		{
			debugViewport.Width = 400.0f;
			debugViewport.Height = 225.0f;
			debugViewport.TopLeftX = fullscreen.Width - debugViewport.Width - 20.0f;
			debugViewport.TopLeftY = 20.0f;
			debugViewport.MinDepth = 0.0f;
			debugViewport.MaxDepth = 1.0f;
		}
		context->RSSetViewports(1, &debugViewport);

		if (!m_isMagnified)
		{
			context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		}

		float aspect = debugViewport.Width / debugViewport.Height;

		m_projMatrix =
			DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(
				m_debugcameraController->GetFov(),
				aspect,
				0.01f,
				1000.0f
			);
		DirectX::SimpleMath::Matrix view = m_debugcameraController->GetView();

		if (skybox && m_isMagnified) skybox->Draw(gameContext, view, m_projMatrix);
		for (const auto& actor : actors) actor->Draw(gameContext, view, m_projMatrix);

		context->RSSetViewports(1, &fullscreen);
	}
	const DirectX::SimpleMath::Matrix DebugDisplayController::GetViewMatrix() const
	{
		return m_debugcameraController->GetView();
	}

	const DirectX::SimpleMath::Matrix DebugDisplayController::GetProjMatrix() const
	{
		return m_projMatrix;
	}

}
