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

        m_debugcameraController->RegisterCamera(
            HEIN::CameraType::Debug,
            []()
            { return std::make_unique<HEIN::DebugCameraMode>(); }
        );

		m_debugcameraController->SetFirstCamera(CameraType::Debug);
	}

	void DebugDisplayController::Update(const GameContext& gameContext)
	{
		if (gameContext.keyboardTracker.pressed.F2) m_isMagnified = !m_isMagnified;
		if (gameContext.keyboardTracker.pressed.F3) m_isVisible = !m_isVisible;

		m_debugUI.Update(gameContext);

		const float deltaTime = static_cast<float>(gameContext.timer.GetElapsedSeconds());

		m_debugcameraController->Update(deltaTime);

		if (m_isMagnified)
		{
			CameraInputState debugInput;
			
			std::pair<int, int> mouseDelta = gameContext.inputManager->GetMouseDelta();
			bool isHeld = gameContext.inputManager->IsDebugDrugHeld(gameContext);

		
			if (isHeld)
			{
				m_virtualMouseX += static_cast<float>(mouseDelta.first);
				m_virtualMouseY += static_cast<float>(mouseDelta.second);
			}

			debugInput.mouseX = m_virtualMouseX;
			debugInput.mouseY = m_virtualMouseY;
			debugInput.movementIntent = gameContext.inputManager->GetDebugMoveIntent(gameContext);
			debugInput.isLeftMouseDown = isHeld;
			debugInput.scrollWheelDelta = gameContext.mouseState.scrollWheelValue;

			m_debugcameraController->ProcessInput(debugInput);
		}
	}

    void DebugDisplayController::Render(
        GameContext& gameContext,
        HEIN::ActorManager& actorManager, 
        Skybox* skybox,
        DirectX::SimpleMath::Matrix mainView,
        DirectX::SimpleMath::Matrix mainProj
    )
    {
        if (!m_isVisible)
        {
            if (gameContext.debugCollisionRenderer != nullptr) gameContext.debugCollisionRenderer->Clear();
            return;
        }

        ID3D11DeviceContext* context = gameContext.deviceResources.GetD3DDeviceContext();
        ID3D11DepthStencilView* dsv = gameContext.deviceResources.GetDepthStencilView();
        D3D11_VIEWPORT fullscreen = gameContext.deviceResources.GetScreenViewport();
        D3D11_VIEWPORT debugViewport;

        if (m_isMagnified)
        {
            debugViewport = fullscreen;
            ID3D11RenderTargetView* rtv = gameContext.deviceResources.GetRenderTargetView();
            const float clearColor[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
            context->ClearRenderTargetView(rtv, clearColor);
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
        context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        float aspect = debugViewport.Width / debugViewport.Height;
        m_projMatrix = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(m_debugcameraController->GetFov(), aspect, 0.01f, 1000.0f);
        DirectX::SimpleMath::Matrix view = m_debugcameraController->GetView();

        if (skybox && m_isMagnified) skybox->Draw(gameContext, view, m_projMatrix);

        actorManager.DrawAll(gameContext, view, m_projMatrix);

        DirectX::BoundingFrustum mainCamFrustum(mainProj, false);
        DirectX::SimpleMath::Matrix mainCamWorld = mainView.Invert();
        mainCamFrustum.Transform(mainCamFrustum, mainCamWorld);
        gameContext.debugRenderer->Begin(view, m_projMatrix);
        gameContext.debugRenderer->DrawFrustum(mainCamFrustum, DirectX::XMVectorSet(1.0f, 1.0f, 0.0f, 1.0f));
        DirectX::BoundingSphere camEye(mainCamWorld.Translation(), 0.3f);
        gameContext.debugRenderer->DrawSphere(camEye, DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f));
        gameContext.debugRenderer->End();

        if (gameContext.debugCollisionRenderer != nullptr)
        {
            gameContext.debugCollisionRenderer->RenderAndFlush(context, gameContext.commonStates, view, m_projMatrix);
        }
        context->RSSetViewports(1, &fullscreen);

        if (m_isMagnified)
        {
            // Safely grab the pointers for the UI
            HEIN::Actor* pPlayer = actorManager.GetActor(m_debugPlayerID);
            HEIN::Actor* pSword = actorManager.GetActor(m_debugSwordID);
            HEIN::Actor* pStage = actorManager.GetActor(m_debugStageID);

            m_debugUI.Draw(pPlayer, pSword, pStage);
        }
    }
	const DirectX::SimpleMath::Matrix DebugDisplayController::GetViewMatrix() const
	{
		return m_debugcameraController->GetView();
	}

	const DirectX::SimpleMath::Matrix DebugDisplayController::GetProjMatrix() const
	{
		return m_projMatrix;
	}

    void DebugDisplayController::SetDebugTargets(HEIN::ActorID playerID, HEIN::ActorID swordID, HEIN::ActorID stageID)
    {
        m_debugPlayerID = playerID;
        m_debugSwordID = swordID;
        m_debugStageID = stageID;
    }


}
